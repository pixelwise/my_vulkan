export VULKAN_SDK="/opt/vulkan"
export VK_ICD_FILENAMES="$VULKAN_SDK/etc/vulkan/icd.d/MoltenVK_icd.json"
export VK_LAYER_PATH="$VULKAN_SDK/etc/vulkan/explicit_layers.d" 
export PATH="$VULKAN_SDK/bin:$PATH"
