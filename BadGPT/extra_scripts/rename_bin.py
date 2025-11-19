Import("env")
# Rename firmware binary using options from the active environment.
# Priority: app_name (preferred) -> custom_prog_name (backward-compatible) -> "BaseApp".

cfg = env.GetProjectConfig()
section = "env:" + env["PIOENV"]

# Safe get: returns default if option is missing (does not raise)
app_name = cfg.get(section, "app_name", None)
custom_name = cfg.get(section, "custom_prog_name", None)

project_name = app_name or custom_name or "BaseApp"
env.Replace(PROGNAME=project_name)