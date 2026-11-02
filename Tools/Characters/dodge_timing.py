"""Shared player dodge blending, in gameplay seconds at the fitted montage rate."""
import unreal as u


def configure_dodge_blends(montage):
    for prop, seconds in [('blend_in', .06), ('blend_out', .20)]:
        blend = montage.get_editor_property(prop)
        blend.set_editor_property('blend_time', seconds)
        blend.set_editor_property('blend_option', u.AlphaBlendOption.CUBIC)
        montage.set_editor_property(prop, blend)
    montage.set_editor_property('blend_out_trigger_time', -1.0)
