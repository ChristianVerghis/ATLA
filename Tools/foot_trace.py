import unreal
def bone_pos(seq, bone, t):
    opts = unreal.AnimPoseEvaluationOptions()
    pose = unreal.AnimPoseExtensions.get_anim_pose_at_time(seq, t, opts)
    return unreal.AnimPoseExtensions.get_bone_pose(pose, bone, unreal.AnimPoseSpaces.WORLD).translation
for n in ['chapa_2', 'bencao']:
    seq = unreal.load_asset('/Game/Anims/Mixamo/MXQ_MX_%s' % n)
    length = seq.get_editor_property('sequence_length')
    out = []
    t = 0.0
    prev = None
    while t <= length:
        fr = bone_pos(seq, 'foot_r', t)
        vz = 0 if prev is None else (fr.z - prev) / 0.05
        out.append('%.2f:z%d,v%d' % (t, fr.z, vz))
        prev = fr.z
        t += 0.05
    unreal.log('FT %s %s' % (n, ' '.join(out)))
unreal.log('FT COMPLETE')
