"""Find an uppercut->punch pair: a hand rising fast (rock comes up) followed
within ~1s by a forward extension (rock is sent)."""
import unreal
OPTS = unreal.AnimPoseEvaluationOptions()
ar = unreal.AssetRegistryHelpers.get_asset_registry()
CLIPS = [n for n in sorted(str(a.asset_name) for a in ar.get_assets_by_path('/Game/Anims/Mixamo', recursive=False))
         if any(k in n.lower() for k in ['punch','hook','elbow','fight','magic_attack','cast_spell'])]
def bone(p, b):
    return unreal.AnimPoseExtensions.get_bone_pose(p, b, unreal.AnimPoseSpaces.WORLD).translation
best = []
for n in CLIPS:
    seq = unreal.load_asset('/Game/Anims/Mixamo/%s.%s' % (n, n))
    if not seq: continue
    L = min(seq.get_editor_property('sequence_length'), 5.0)
    ts, up, fwd = [], [], []
    t = 0.05
    prev = None
    while t < L:
        p = unreal.AnimPoseExtensions.get_anim_pose_at_time(seq, t, OPTS)
        pel = bone(p, 'pelvis')
        h = bone(p, 'hand_r') - pel
        ts.append(t)
        up.append(0.0 if prev is None else (h.z - prev.z) / 0.05)
        fwd.append(unreal.Vector(h.x, h.y, 0).length())
        prev = h
        t += 0.05
    for i in range(2, len(ts)-2):
        if up[i] < 180: continue          # hand rising hard = the uppercut
        for j in range(i+2, min(i+22, len(ts))):   # punch within ~1s after
            gain = fwd[j] - fwd[i]
            if gain > 25 and fwd[j] > 45:
                best.append((up[i], ts[i], ts[j], n))
                break
best.sort(reverse=True)
for b in best[:6]:
    unreal.log("UPC rise=%.0f up@%.2f punch@%.2f  %s" % (b[0], b[1], b[2], b[3].replace('MXQ_MX_','')))
unreal.log("UPC COMPLETE")
