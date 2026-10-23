"""Run in Unreal: captures current spring behaviour without saving content assets."""
import os
import unreal as u

label = os.environ.get("IC_SECONDARY_REVIEW_LABEL", "Current")
passed = u.ICCharacterSetupLibrary.review_secondary_motion(label, os.environ.get("IC_SECONDARY_REVIEW_CAPTURE", "0") == "1")
u.log("IC_SECONDARY_MOTION_RESULT label=%s passed=%s" % (label, passed))
if not passed:
    raise RuntimeError("Secondary motion regression failed; inspect Saved/SecondaryMotionReview/" + label)
