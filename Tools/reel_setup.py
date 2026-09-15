"""Prepare a clean PIE session for recording the gameplay reel.
- floating PIE window 640x360 points (1280x720 px on Retina) so plain `Shot` frames are 720p
- background CPU throttle off (otherwise an unfocused editor runs at ~3 fps)
- begins play
LevelEditorPlaySettings isn't exposed to Python as a class, but its CDO accepts
exact-case property names through reflection. The play MODE itself is protected:
it must already be PlayMode_InEditorFloating in EditorPerProjectUserSettings.ini.
"""
import unreal

W, H, X, Y = 640, 360, 40, 60
ps = unreal.find_object(None, "/Script/UnrealEd.Default__LevelEditorPlaySettings")
for name, val in (("NewWindowWidth", W), ("NewWindowHeight", H), ("CenterNewWindow", False), ("NewWindowPosition", unreal.IntPoint(X, Y))):
    try:
        ps.set_editor_property(name, val)
    except Exception as e:  # noqa
        unreal.log("REELSETUP FAILED %s: %s" % (name, e))
perf = unreal.find_object(None, "/Script/UnrealEd.Default__EditorPerformanceSettings")
try:
    perf.set_editor_property("bThrottleCPUWhenNotForeground", False)
except Exception as e:  # noqa
    unreal.log("REELSETUP throttle: %s" % e)
unreal.log("REELSETUP window %dx%d throttle=%s" % (W, H, perf.get_editor_property("bThrottleCPUWhenNotForeground")))

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if les.is_in_play_in_editor():
    unreal.log("REELSETUP already in PIE")
else:
    les.editor_request_begin_play()
    unreal.log("REELSETUP begin play requested")
