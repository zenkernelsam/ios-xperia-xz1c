// SPDX-License-Identifier: BSD-2-Clause
// One-way native command lifecycle; failures never become completed work.
#pragma once
#include <stdint.h>
namespace SweetMetal {
enum class QueueStatus:uint32_t {Created,Enqueued,Committed,Scheduled,Completed,Error};
struct QueueState {
    QueueStatus status=QueueStatus::Created;bool encoderOpen=false,invalid=false;
    bool beginEncoder(){if(status!=QueueStatus::Created || encoderOpen || invalid)return false;encoderOpen=true;return true;}
    bool endEncoder(){if(!encoderOpen || status!=QueueStatus::Created)return false;encoderOpen=false;return true;}
    bool enqueue(){if(status!=QueueStatus::Created || encoderOpen || invalid)return false;status=QueueStatus::Enqueued;return true;}
    bool commit(){if((status!=QueueStatus::Created && status!=QueueStatus::Enqueued) || encoderOpen || invalid)return false;status=QueueStatus::Committed;return true;}
    bool schedule(){if(status!=QueueStatus::Committed)return false;status=QueueStatus::Scheduled;return true;}
    bool finish(bool success){if(status!=QueueStatus::Scheduled)return false;status=success?QueueStatus::Completed:QueueStatus::Error;return true;}
};
}
