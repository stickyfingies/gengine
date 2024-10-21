# This procedure takes SPIR-V and makes ES-3.0 compliant GLSL
INPUT=vk.frag.spv
OUTPUT=vk.frag.gles.glsl
spirv-cross $INPUT --output $OUTPUT --es --version 300