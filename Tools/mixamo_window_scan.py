"""Scan retargeted Mixamo clips for wire-able moments:
  THRUST  - hand extends forward fast (punches)
  KICK    - foot speed peak
Prints candidates so windows are picked from measurement, then pose-verified.
"""
import unreal

OPTS = unreal.AnimPoseEvaluationOptions()
ar = unreal.AssetRegistryHelpers.get_asset_registry()

WANT = ["Cross_Punch", "Hook", "Illegal_Elbow_Punch", "Punch_Combo", "Fist_Fight_A", "Fist_Fight_B",
        "Roundhouse_Kick", "Flying_Kick", "Hurricane_Kick", "Leg_Sweep", "MixamoBase",
        "armada", "bencao", "chapa", "martelo", "au", "ginga", "esquiva",
        "1H_cast_spell", "2H_cast_spell", "1h_magic", "2h_magic"]

names = sorted(str(a.asset_name) for a in ar.get_assets_by_path("/Game/Anims/Mixamo", recursive=False))


def bone(pose, b):
    return unreal.AnimPoseExtensions.get_bone_pose(pose, b, unreal.AnimPoseSpaces.WORLD).translation


for n in names:
    if not any(w.lower() in n.lower() for w in WANT):
        continue
    seq = unreal.load_asset("/Game/Anims/Mixamo/%s.%s" % (n, n))
    if not seq:
        continue
    L = seq.get_editor_property("sequence_length")
    ts, fwd, hs, fs = [], [], [], []
    prev_h, prev_f = None, None
    t = 0.05
    while t < L:
        p = unreal.AnimPoseExtensions.get_anim_pose_at_time(seq, t, OPTS)
        pel = bone(p, "pelvis")
        h = bone(p, "hand_r") - pel
        f = bone(p, "foot_r") - pel
        ts.append(t)
        fwd.append(unreal.Vector(h.x, h.y, 0).length())
        hs.append(0.0 if prev_h is None else (h - prev_h).length() / 0.05)
        fs.append(0.0 if prev_f is None else (f - prev_f).length() / 0.05)
        prev_h, prev_f = h, f
        t += 0.05

    def peaks(arr, thresh):
        out = []
        for i in range(1, len(arr) - 1):
            if arr[i] > thresh and arr[i] >= arr[i - 1] and arr[i] >= arr[i + 1]:
                out.append((ts[i], arr[i]))
        out.sort(key=lambda x: -x[1])
        keep = []
        for e in out:
            if all(abs(e[0] - k[0]) > 0.25 for k in keep):
                keep.append(e)
        return keep[:3]

    # thrust = max forward reach moment
    reach_i = max(range(len(fwd)), key=lambda i: fwd[i]) if fwd else 0
    hp = peaks(hs, 250)
    fp = peaks(fs, 250)
    unreal.log("MWS %-42s len=%.1f reach=%.0f@%.2f hand=%s foot=%s" % (
        n.replace("MXQ_MX_", "").replace("_Anim_mixamo_com", ""), L, fwd[reach_i], ts[reach_i],
        ",".join("%.2f@%.0f" % p for p in hp) or "-",
        ",".join("%.2f@%.0f" % p for p in fp) or "-"))
unreal.log("MWS COMPLETE")
