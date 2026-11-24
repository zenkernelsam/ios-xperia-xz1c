#include <assert.h>

#include "../../xnu/support/LilacWledDutyTransaction.h"

struct fake_wled {
    uint8_t control[256], sink[256], staged[4];
    unsigned reads, writes, waits, fail_read, fail_write, fail_wait, skip_latch, fault_on_latch, stuck_sync;
};

static const uint8_t duty_offsets[] = {0x57, 0x58, 0x67, 0x68};

static struct fake_wled stock_state(void)
{
    struct fake_wled fake = {0};
    fake.control[0x46] = 0x80;
    fake.sink[0x46] = 0x30;
    fake.sink[0x50] = fake.sink[0x60] = 0x80;
    fake.sink[0x57] = fake.sink[0x67] = 0x34;
    fake.sink[0x58] = fake.sink[0x68] = 2;
    return fake;
}

static int read_register(void *context, unsigned ppid, unsigned offset, uint8_t *value)
{
    struct fake_wled *fake = context;
    assert(offset < 256 && (ppid == 0x3d8 || ppid == 0x3d9));
    if (++fake->reads == fake->fail_read) {
        return -1;
    }
    *value = ppid == 0x3d8 ? fake->control[offset] : fake->sink[offset];
    return 0;
}

static int write_sink(void *context, uint8_t offset, uint8_t value)
{
    struct fake_wled *fake = context;
    const unsigned index = fake->writes++;
    assert(index < 6);
    if (index < 4) {
        assert(offset == duty_offsets[index]);
        fake->staged[index] = value;
    } else if (index == 4) {
        assert(offset == 0x47 && value == 0x0f);
        fake->sink[offset] = value;
    } else {
        assert(offset == 0x47 && value == 0 && fake->waits == 1);
        if (!fake->stuck_sync) {
            fake->sink[offset] = 0;
        }
        if (!fake->skip_latch) {
            for (unsigned channel = 0; channel < 4; ++channel) {
                fake->sink[duty_offsets[channel]] = fake->staged[channel];
            }
        }
        if (fake->fault_on_latch) {
            fake->control[0x08] = 1;
        }
    }
    return fake->writes == fake->fail_write ? -1 : 0;
}

static int wait_us(void *context, uint16_t delay)
{
    struct fake_wled *fake = context;
    assert(delay == 1000 && fake->writes == 5 && fake->sink[0x47] == 0x0f);
    ++fake->waits;
    return fake->fail_wait ? -1 : 0;
}

static const struct lilac_wled_duty_io fake_io = {read_register, write_sink, wait_us};

static void expect_locked(struct lilac_wled_transaction *transaction, struct fake_wled *fake)
{
    const unsigned reads = fake->reads, writes = fake->writes;
    assert(lilac_wled_transact_duty(transaction, &fake_io, fake, 1000, 2000) == LILAC_DUTY_TRANSACTION_LOCKED);
    assert(fake->reads == reads && fake->writes == writes);
}

int main(void)
{
    struct lilac_wled_transaction transaction = {0};
    struct fake_wled fake = stock_state();
    assert(lilac_wled_transact_duty(&transaction, &fake_io, &fake, 564, 2000) == LILAC_DUTY_TRANSACTION_UNCHANGED);
    assert(fake.reads == 19 && fake.writes == 0);
    fake = stock_state();
    fake.sink[0x47] = 1;
    assert(lilac_wled_transact_duty(&transaction, &fake_io, &fake, 564, 2000) == LILAC_DUTY_TRANSACTION_REFUSED);
    assert(fake.reads == 19 && fake.writes == 0 && !transaction.uncertain);
    fake = stock_state();
    assert(lilac_wled_transact_duty(&transaction, &fake_io, &fake, 1000, 2000) == LILAC_DUTY_TRANSACTION_VERIFIED);
    assert(fake.reads == 38 && fake.writes == 6 && fake.waits == 1 && !transaction.uncertain);
    assert(fake.sink[0x57] == 0xe8 && fake.sink[0x58] == 3);
    assert(fake.sink[0x67] == 0xe8 && fake.sink[0x68] == 3);
    assert(fake.control[0x46] == 0x80 && fake.sink[0x46] == 0x30);
    assert(lilac_wled_transact_duty(&transaction, &fake_io, &fake, 1000, 2000) == LILAC_DUTY_TRANSACTION_UNCHANGED);

    for (unsigned failed = 1; failed <= 38; ++failed) {
        transaction.uncertain = 0;
        fake = stock_state();
        fake.fail_read = failed;
        const enum lilac_wled_transaction_result result =
            lilac_wled_transact_duty(&transaction, &fake_io, &fake, 1000, 2000);
        if (failed <= 19) {
            assert(result == LILAC_DUTY_TRANSACTION_REFUSED && fake.writes == 0 && !transaction.uncertain);
        } else {
            assert(result == LILAC_DUTY_TRANSACTION_UNCERTAIN && fake.writes == 6);
            expect_locked(&transaction, &fake);
        }
        assert(fake.reads == failed);
    }

    for (unsigned failed = 1; failed <= 6; ++failed) {
        transaction.uncertain = 0;
        fake = stock_state();
        fake.fail_write = failed;
        assert(lilac_wled_transact_duty(&transaction, &fake_io, &fake, 1000, 2000) == LILAC_DUTY_TRANSACTION_UNCERTAIN);
        assert(fake.writes == failed && fake.reads == 19);
        expect_locked(&transaction, &fake);
    }

    for (unsigned scenario = 0; scenario < 4; ++scenario) {
        transaction.uncertain = 0;
        fake = stock_state();
        fake.fail_wait = scenario == 0;
        fake.skip_latch = scenario == 1;
        fake.fault_on_latch = scenario == 2;
        fake.stuck_sync = scenario == 3;
        assert(lilac_wled_transact_duty(&transaction, &fake_io, &fake, 1000, 2000) == LILAC_DUTY_TRANSACTION_UNCERTAIN);
        assert(fake.writes == (fake.fail_wait ? 5u : 6u));
        expect_locked(&transaction, &fake);
    }
}
