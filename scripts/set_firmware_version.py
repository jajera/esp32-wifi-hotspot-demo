Import("env")
import os

version = os.getenv("FIRMWARE_VERSION", "1.0.0-phase1")
# Quoted string macro; hyphens in version tags must stay inside string literals.
env.Append(CPPDEFINES=[("FIRMWARE_VERSION", '\\"%s\\"' % version)])
