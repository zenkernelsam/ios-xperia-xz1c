"""Owned child processes: cancellation closes a Windows job and its whole tree."""
import ctypes
import os
import signal
import subprocess
import time

if os.name == "nt":
    from ctypes import wintypes as w
    k = ctypes.WinDLL("kernel32", use_last_error=True)
    class BasicLimits(ctypes.Structure):
        _fields_ = [("process_time", ctypes.c_int64), ("job_time", ctypes.c_int64),
                    ("flags", w.DWORD), ("minimum_ws", ctypes.c_size_t), ("maximum_ws", ctypes.c_size_t),
                    ("active_processes", w.DWORD), ("affinity", ctypes.c_size_t),
                    ("priority", w.DWORD), ("scheduling", w.DWORD)]
    class IOCounters(ctypes.Structure):
        _fields_ = [(name, ctypes.c_uint64) for name in ("reads", "writes", "other", "read_bytes", "write_bytes", "other_bytes")]
    class ExtendedLimits(ctypes.Structure):
        _fields_ = [("basic", BasicLimits), ("io", IOCounters), ("process_memory", ctypes.c_size_t),
                    ("job_memory", ctypes.c_size_t), ("peak_process_memory", ctypes.c_size_t), ("peak_job_memory", ctypes.c_size_t)]
    class ThreadEntry(ctypes.Structure):
        _fields_ = [("size", w.DWORD), ("usage", w.DWORD), ("tid", w.DWORD), ("pid", w.DWORD),
                    ("base_priority", w.LONG), ("delta_priority", w.LONG), ("flags", w.DWORD)]
    def api(name, args, result):
        f = getattr(k, name); f.argtypes = args; f.restype = result; return f
    create_job = api("CreateJobObjectW", [ctypes.c_void_p, w.LPCWSTR], w.HANDLE)
    set_job = api("SetInformationJobObject", [w.HANDLE, ctypes.c_int, ctypes.c_void_p, w.DWORD], w.BOOL)
    assign_job = api("AssignProcessToJobObject", [w.HANDLE, w.HANDLE], w.BOOL)
    terminate_job = api("TerminateJobObject", [w.HANDLE, w.UINT], w.BOOL)
    close_handle = api("CloseHandle", [w.HANDLE], w.BOOL)
    snapshot = api("CreateToolhelp32Snapshot", [w.DWORD, w.DWORD], w.HANDLE)
    first_thread = api("Thread32First", [w.HANDLE, ctypes.POINTER(ThreadEntry)], w.BOOL)
    next_thread = api("Thread32Next", [w.HANDLE, ctypes.POINTER(ThreadEntry)], w.BOOL)
    open_thread = api("OpenThread", [w.DWORD, w.BOOL, w.DWORD], w.HANDLE)
    resume_thread = api("ResumeThread", [w.HANDLE], w.DWORD)
    create_mutex = api("CreateMutexW", [ctypes.c_void_p, w.BOOL, w.LPCWSTR], w.HANDLE)
    wait_handle = api("WaitForSingleObject", [w.HANDLE, w.DWORD], w.DWORD)
    release_mutex = api("ReleaseMutex", [w.HANDLE], w.BOOL)
    class ProcessEntry(ctypes.Structure):
        _fields_ = [("size", w.DWORD), ("usage", w.DWORD), ("pid", w.DWORD), ("heap", ctypes.c_size_t),
                    ("module", w.DWORD), ("threads", w.DWORD), ("parent", w.DWORD),
                    ("priority", w.LONG), ("flags", w.DWORD), ("exe", w.WCHAR * 260)]
    first_process = api("Process32FirstW", [w.HANDLE, ctypes.POINTER(ProcessEntry)], w.BOOL)
    next_process = api("Process32NextW", [w.HANDLE, ctypes.POINTER(ProcessEntry)], w.BOOL)

    class Job:
        def __init__(self):
            self.handle = create_job(None, None)
            if not self.handle: raise ctypes.WinError(ctypes.get_last_error())
            limits = ExtendedLimits(); limits.basic.flags = 0x2000  # KILL_ON_JOB_CLOSE
            if not set_job(self.handle, 9, ctypes.byref(limits), ctypes.sizeof(limits)):
                self.close(); raise ctypes.WinError(ctypes.get_last_error())
        def attach(self, process):
            if not assign_job(self.handle, int(process._handle)):
                raise ctypes.WinError(ctypes.get_last_error())
            snap = snapshot(4, 0)  # TH32CS_SNAPTHREAD
            if snap == ctypes.c_void_p(-1).value: raise ctypes.WinError(ctypes.get_last_error())
            try:
                item = ThreadEntry(); item.size = ctypes.sizeof(item)
                found = first_thread(snap, ctypes.byref(item))
                while found:
                    if item.pid == process.pid:
                        th = open_thread(2, False, item.tid)  # THREAD_SUSPEND_RESUME
                        if not th: raise ctypes.WinError(ctypes.get_last_error())
                        try:
                            if resume_thread(th) == 0xffffffff: raise ctypes.WinError(ctypes.get_last_error())
                        finally: close_handle(th)
                        return
                    found = next_thread(snap, ctypes.byref(item))
                raise RuntimeError("Suspended primary thread not found")
            finally: close_handle(snap)
        def close(self):
            if self.handle:
                close_handle(self.handle); self.handle = None

def active_tool_processes():
    if os.name != "nt": return []
    snap = snapshot(2, 0)
    if snap == ctypes.c_void_p(-1).value: raise ctypes.WinError(ctypes.get_last_error())
    result = []
    try:
        item = ProcessEntry(); item.size = ctypes.sizeof(item)
        found = first_process(snap, ctypes.byref(item))
        while found:
            if item.exe.lower() in ("fastboot.exe", "xperable.exe"):
                result.append({"pid": item.pid, "exe": item.exe})
            found = next_process(snap, ctypes.byref(item))
    finally: close_handle(snap)
    return result

class LiveLock:
    """An abandoned controller releases its OS lock automatically."""
    def __init__(self, path, name="Local\\lilac-loop-BH905ND69G"):
        self.handle = None; self.file = None
        if os.name == "nt":
            self.handle = create_mutex(None, False, name)
            if not self.handle: raise ctypes.WinError(ctypes.get_last_error())
            if wait_handle(self.handle, 0) not in (0, 0x80):
                close_handle(self.handle); self.handle = None
                raise RuntimeError("Another lilac loop owns the device")
        else:
            import fcntl
            self.file = open(path, "a")
            try: fcntl.flock(self.file, fcntl.LOCK_EX | fcntl.LOCK_NB)
            except BaseException: self.file.close(); raise
    def close(self):
        if self.handle: release_mutex(self.handle); close_handle(self.handle); self.handle = None
        if self.file: self.file.close(); self.file = None

class OwnedProcess:
    def __init__(self, argv, *, cwd, env=None, stdout=subprocess.PIPE, stderr=subprocess.PIPE):
        self.job = None
        self.closed = False
        kwargs = dict(cwd=str(cwd), env=env, stdin=subprocess.DEVNULL, stdout=stdout, stderr=stderr)
        if os.name == "nt":
            self.job = Job()
            kwargs["creationflags"] = 0x08000000 | 0x00000004  # NO_WINDOW | SUSPENDED
            si = subprocess.STARTUPINFO(); si.dwFlags = subprocess.STARTF_USESHOWWINDOW; si.wShowWindow = 0
            kwargs["startupinfo"] = si
        else: kwargs["start_new_session"] = True
        try:
            self.process = subprocess.Popen([str(s) for s in argv], **kwargs)
            if self.job: self.job.attach(self.process)
        except BaseException:
            if hasattr(self, "process"):
                self.process.kill(); self.process.wait(timeout=5)
            if self.job: self.job.close()
            raise
    def close(self):
        if self.closed: return
        self.closed = True
        if self.job: self.job.close()
        elif self.process.poll() is None:
            try: os.killpg(self.process.pid, signal.SIGKILL)
            except ProcessLookupError: pass
        if self.process.poll() is None:
            try: self.process.wait(timeout=5)
            except subprocess.TimeoutExpired: self.process.kill(); self.process.wait(timeout=5)
        for pipe in (self.process.stdin, self.process.stdout, self.process.stderr):
            if pipe is not None: pipe.close()
    def wait_output(self, timeout):
        try:
            out, err = self.process.communicate(timeout=timeout)
            return self.process.returncode, out or b"", err or b""
        finally: self.close()
    def __enter__(self): return self
    def __exit__(self, *_): self.close()

def run(argv, *, cwd, timeout=20, env=None):
    with OwnedProcess(argv, cwd=cwd, env=env) as process:
        code, out, err = process.wait_output(timeout)
    return code, out.decode("utf-8", "replace"), err.decode("utf-8", "replace")
