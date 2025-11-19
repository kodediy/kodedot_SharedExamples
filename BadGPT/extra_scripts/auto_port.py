Import("env")

# Auto-detect serial port for upload and monitor if not explicitly set
try:
    # Only override when user didn't set a concrete value (e.g. not "auto")
    opt = env.GetProjectOption("upload_port", None)
    if not opt or opt == "auto":
        port = env.AutodetectUploadPort()
        if port:
            env.Replace(UPLOAD_PORT=port)
            # Keep monitor in sync with upload port unless overridden
            if not env.GetProjectOption("monitor_port", None):
                env.Replace(MONITOR_PORT=port)
        else:
            print("[auto_port] No serial device detected; leaving UPLOAD_PORT = auto")
except Exception as e:
    print("[auto_port] Warning: could not autodetect upload/monitor port:", e)
