"""Single Sony descriptor/claim observation. No Fastboot bulk writes or reset."""
import argparse
import hashlib
import json
import os
from pathlib import Path

from boot_once import SERIAL, VID, PID
from procguard import LiveLock, active_tool_processes


def inspect_device(device, util, claim=False):
    report = dict(serial=SERIAL, fastboot_download_commands=0, fastboot_boot_commands=0,
                  reset_commands=0, set_configuration_calls=0, bulk_reads=0, bulk_writes=0,
                  requested_claim=claim, claim_attempts=0, release_attempts=0)
    phase = 'identity'; claimed = False
    try:
        identity = util.get_string(device, device.iSerialNumber)
        assert identity == SERIAL, 'Target Sony identity differs'
        report['observed_serial'] = identity
        phase = 'active_configuration'; config = device.get_active_configuration()
        report['active_configuration'] = config.bConfigurationValue
        phase = 'interface_descriptor'; interface = config[(0, 0)]
        assert (interface.bInterfaceClass, interface.bInterfaceSubClass, interface.bInterfaceProtocol)==(255,66,3)
        endpoints = {e.bEndpointAddress:e for e in interface}
        assert {1,129} <= endpoints.keys() and all(util.endpoint_type(endpoints[e].bmAttributes)==util.ENDPOINT_TYPE_BULK for e in (1,129))
        report['bulk_endpoints'] = [dict(address=e, attributes=endpoints[e].bmAttributes, max_packet=endpoints[e].wMaxPacketSize) for e in (1,129)]
        if claim:
            phase = 'claim_interface'; report['claim_attempts']=1
            util.claim_interface(device,0); claimed=True
        report['result'] = 'OPEN_AVAILABLE' if claim else 'DESCRIPTORS_AVAILABLE'
    except Exception as error:
        report.update(result='REFUSED', failed_phase=phase, error=str(error), exception_type=type(error).__name__, errno=getattr(error,'errno',None))
    finally:
        if claimed:
            report['release_attempts']=1
            try: util.release_interface(device,0)
            except Exception as error: report.update(result='REFUSED', release_error=str(error))
        try: util.dispose_resources(device)
        except Exception as error: report.update(result='REFUSED', dispose_error=str(error))
    return report


def observe(output, claim=False):
    output=Path(output).resolve(); assert output.is_relative_to(Path(os.environ['TEMP']).resolve()) and not output.exists()
    assert not active_tool_processes(), 'Another boot/patch process is active'
    output.mkdir(); lock=LiveLock(output/'device.lock')
    report=dict(serial=SERIAL, fastboot_download_commands=0, fastboot_boot_commands=0, reset_commands=0,
                bulk_reads=0, bulk_writes=0, set_configuration_calls=0, requested_claim=claim)
    phase='backend'
    try:
        import libusb_package
        import usb.core
        import usb.util
        import usb.backend.libusb1 as backend
        lib=backend.get_backend(find_library=libusb_package.find_library); assert lib is not None
        phase='enumeration'; devices=list(usb.core.find(find_all=True,idVendor=VID,idProduct=PID,backend=lib))
        assert len(devices)==1, 'Require exactly one Sony 0fce:0dde'
        report=inspect_device(devices[0],usb.util,claim)
    except Exception as error:
        report.update(result='REFUSED',failed_phase=phase,error=str(error),exception_type=type(error).__name__)
    finally:
        lock.close(); report.update(source_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                                    audited_sender_sha256=hashlib.sha256(Path(__file__).with_name('boot_once.py').read_bytes()).hexdigest(),
                                    observation_does_not_authorize_replaying_a_failed_trial=True)
        (output/'observation.json').open('xb').write((json.dumps(report,indent=2)+'\n').encode())
    return report


if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--output',required=True);parser.add_argument('--claim-once',action='store_true')
    args=parser.parse_args();print(json.dumps(observe(args.output,args.claim_once)))
