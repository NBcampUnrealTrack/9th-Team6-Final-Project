"""Convert the 13 character post-process graphs to Kawaii Physics.

Run in Unreal Python. Default is an inspection plan; IC_APPLY_KAWAII=1 applies
and saves the graphs after package backups. Morph/constraint graph nodes remain.
"""
import os
import unreal as u

result = u.ICCharacterSetupLibrary.configure_kawaii_secondary_motion(os.environ.get("IC_APPLY_KAWAII") == "1")
u.log("IC_KAWAII_SETUP_RESULT " + result)
if result.startswith("ERROR:"):
    raise RuntimeError(result)
