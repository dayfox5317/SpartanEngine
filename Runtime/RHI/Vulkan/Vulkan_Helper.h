/*
Copyright(c) 2016-2019 Panos Karabelas

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
copies of the Software, and to permit persons to whom the Software is furnished
to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

//= IMPLEMENTATION ===============
#include "../RHI_Implementation.h"
#ifdef API_GRAPHICS_VULKAN
//================================

//= INCLUDES =====================
#include <optional>
#include <set>
#include "../../Logging/Log.h"
#include "../../Core/Settings.h"
#include "../../Math/MathHelper.h"
#include "../RHI_Device.h"
//================================

namespace Spartan::vulkan_helper
{
	inline void log_available_extensions()
	{
		uint32_t extension_count = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
		std::vector<VkExtensionProperties> extensions(extension_count);
		vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extensions.data());
		for (const auto& extension : extensions)
		{
			LOGF_INFO("%s", extension.extensionName);
		}
	}

	inline bool check_validation_layers(RHI_Device* rhi_device)
	{
		uint32_t layer_count;
		vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

		std::vector<VkLayerProperties> available_layers(layer_count);
		vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

		for (auto layer_name : rhi_device->GetContext()->validation_layers)
		{
			for (const auto& layer_properties : available_layers)
			{
				if (strcmp(layer_name, layer_properties.layerName) == 0)
					return true;
			}
		}

		return false;
	}

	inline uint32_t GetMemoryType(VkPhysicalDevice device, VkMemoryPropertyFlags properties, uint32_t type_bits)
	{
		VkPhysicalDeviceMemoryProperties prop;
		vkGetPhysicalDeviceMemoryProperties(device, &prop);
		for (uint32_t i = 0; i < prop.memoryTypeCount; i++)
			if ((prop.memoryTypes[i].propertyFlags & properties) == properties && type_bits & (1 << i))
				return i;

		return 0xFFFFFFFF; // Unable to find memoryType
	}

	inline const char* result_to_string(const VkResult result)
	{
		switch (result)
		{
		case VK_NOT_READY:											return "VK_NOT_READY";
		case VK_TIMEOUT:											return "VK_TIMEOUT";
		case VK_EVENT_SET:											return "VK_EVENT_SET";
		case VK_EVENT_RESET:										return "VK_EVENT_RESET";
		case VK_INCOMPLETE:											return "VK_INCOMPLETE";
		case VK_ERROR_OUT_OF_HOST_MEMORY:							return "VK_ERROR_OUT_OF_HOST_MEMORY";
		case VK_ERROR_OUT_OF_DEVICE_MEMORY:							return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
		case VK_ERROR_INITIALIZATION_FAILED:						return "VK_ERROR_INITIALIZATION_FAILED";
		case VK_ERROR_DEVICE_LOST:									return "VK_ERROR_DEVICE_LOST";
		case VK_ERROR_MEMORY_MAP_FAILED:							return "VK_ERROR_MEMORY_MAP_FAILED";
		case VK_ERROR_LAYER_NOT_PRESENT:							return "VK_ERROR_LAYER_NOT_PRESENT";
		case VK_ERROR_EXTENSION_NOT_PRESENT:						return "VK_ERROR_EXTENSION_NOT_PRESENT";
		case VK_ERROR_FEATURE_NOT_PRESENT:							return "VK_ERROR_FEATURE_NOT_PRESENT";
		case VK_ERROR_INCOMPATIBLE_DRIVER:							return "VK_ERROR_INCOMPATIBLE_DRIVER";
		case VK_ERROR_TOO_MANY_OBJECTS:								return "VK_ERROR_TOO_MANY_OBJECTS";
		case VK_ERROR_FORMAT_NOT_SUPPORTED:							return "VK_ERROR_FORMAT_NOT_SUPPORTED";
		case VK_ERROR_FRAGMENTED_POOL:								return "VK_ERROR_FRAGMENTED_POOL";
		case VK_ERROR_OUT_OF_POOL_MEMORY:							return "VK_ERROR_OUT_OF_POOL_MEMORY";
		case VK_ERROR_INVALID_EXTERNAL_HANDLE:						return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
		case VK_ERROR_SURFACE_LOST_KHR:								return "VK_ERROR_SURFACE_LOST_KHR";
		case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:						return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
		case VK_SUBOPTIMAL_KHR:										return "VK_SUBOPTIMAL_KHR";
		case VK_ERROR_OUT_OF_DATE_KHR:								return "VK_ERROR_OUT_OF_DATE_KHR";
		case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:						return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
		case VK_ERROR_VALIDATION_FAILED_EXT:						return "VK_ERROR_VALIDATION_FAILED_EXT";
		case VK_ERROR_INVALID_SHADER_NV:							return "VK_ERROR_INVALID_SHADER_NV";
		case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT: return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
		case VK_ERROR_FRAGMENTATION_EXT:							return "VK_ERROR_FRAGMENTATION_EXT";
		case VK_ERROR_NOT_PERMITTED_EXT:							return "VK_ERROR_NOT_PERMITTED_EXT";
		case VK_ERROR_INVALID_DEVICE_ADDRESS_EXT:					return "VK_ERROR_INVALID_DEVICE_ADDRESS_EXT";
		}

		return "Unknown error code";
	}

	namespace debug_callback
	{
		static VKAPI_ATTR VkBool32 VKAPI_CALL callback(
			VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
			VkDebugUtilsMessageTypeFlagsEXT message_type,
			const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data,
			void* p_user_data
		){
			auto type = Spartan::Log_Info;
			type = message_severity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT ? Spartan::Log_Warning : type;
			type = message_severity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT ? Spartan::Log_Error : type;

			Spartan::Log::m_log_to_file = true;
			Spartan::Log::m_caller_name = "Vulkan";
			Spartan::Log::Write(p_callback_data->pMessage, type);
			Spartan::Log::m_caller_name = "";

			return VK_FALSE;
		}

		inline VkResult create(RHI_Device* rhi_device, const VkDebugUtilsMessengerCreateInfoEXT* create_info)
		{
			if (const auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(rhi_device->GetContext()->instance, "vkCreateDebugUtilsMessengerEXT")))
				return func(rhi_device->GetContext()->instance, create_info, nullptr, &rhi_device->GetContext()->callback_handle);

			return VK_ERROR_EXTENSION_NOT_PRESENT;
		}

		inline void destroy(Spartan::RHI_Context* context)
		{
			if (const auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(context->instance, "vkDestroyDebugUtilsMessengerEXT")))
			{
				func(context->instance, context->callback_handle, nullptr);
			}
		}
	}

	namespace swap_chain
	{
		inline SwapChainSupportDetails check_surface_compatibility(RHI_Device* rhi_device, const VkSurfaceKHR surface)
		{
			SwapChainSupportDetails details;
			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(rhi_device->GetContext()->device_physical, surface, &details.capabilities);

			uint32_t format_count;
			vkGetPhysicalDeviceSurfaceFormatsKHR(rhi_device->GetContext()->device_physical, surface, &format_count, nullptr);

			if (format_count != 0)
			{
				details.formats.resize(format_count);
				vkGetPhysicalDeviceSurfaceFormatsKHR(rhi_device->GetContext()->device_physical, surface, &format_count, details.formats.data());
			}

			uint32_t present_mode_count;
			vkGetPhysicalDeviceSurfacePresentModesKHR(rhi_device->GetContext()->device_physical, surface, &present_mode_count, nullptr);

			if (present_mode_count != 0)
			{
				details.present_modes.resize(present_mode_count);
				vkGetPhysicalDeviceSurfacePresentModesKHR(rhi_device->GetContext()->device_physical, surface, &present_mode_count, details.present_modes.data());
			}

			return details;
		}

		inline VkPresentModeKHR choose_present_mode(const std::vector<VkPresentModeKHR>& present_modes)
		{
			VkPresentModeKHR best_mode = VK_PRESENT_MODE_FIFO_KHR;

			for (const auto& present_mode : present_modes)
			{
				if (present_mode == VK_PRESENT_MODE_MAILBOX_KHR)
				{
					return present_mode;
				}
				else if (present_mode == VK_PRESENT_MODE_IMMEDIATE_KHR)
				{
					best_mode = present_mode;
				}
			}

			return best_mode;
		}

		inline VkSurfaceFormatKHR choose_format(const VkFormat prefered_format, const std::vector<VkSurfaceFormatKHR>& availableFormats)
		{
			VkColorSpaceKHR color_space = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

			if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED)
			{
				LOG_ERROR("Failed to find format");
				return { prefered_format, color_space };
			}

			for (const auto& availableFormat : availableFormats)
			{
				if (availableFormat.format == prefered_format && availableFormat.colorSpace == color_space)
				{
					return availableFormat;
				}
			}

			return availableFormats[0];
		}

		inline VkExtent2D choose_extent(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t prefered_width, uint32_t prefered_height)
		{
			using namespace Spartan::Math::Helper;

			VkExtent2D actual_extent	= { prefered_width, prefered_height };
			actual_extent.width			= Clamp(prefered_width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
			actual_extent.height		= Clamp(prefered_height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

			return actual_extent;
		}
	}

	namespace physical_device
	{
		inline QueueFamilyIndices get_family_indices(RHI_Device* rhi_device, VkSurfaceKHR& surface, const VkPhysicalDevice& _physical_device)
		{
			QueueFamilyIndices indices;

			uint32_t queue_family_count = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(_physical_device, &queue_family_count, nullptr);

			std::vector<VkQueueFamilyProperties> queue_family_properties(queue_family_count);
			vkGetPhysicalDeviceQueueFamilyProperties(_physical_device, &queue_family_count, queue_family_properties.data());
			
			int i = 0;
			for (const auto& queue_family_property : queue_family_properties)
			{
				VkBool32 present_support = false;
				vkGetPhysicalDeviceSurfaceSupportKHR(_physical_device, i, surface, &present_support);
				if (queue_family_property.queueCount > 0)
				{
					indices.present_family = i;
				}

				if (queue_family_property.queueCount > 0 && queue_family_property.queueFlags & VK_QUEUE_GRAPHICS_BIT)
				{
					indices.graphics_family = i;
				}

				if (queue_family_property.queueCount > 0 && queue_family_property.queueFlags & VK_QUEUE_TRANSFER_BIT) 
				{
					indices.copy_family = i;
				}

				if (indices.IsComplete())
					break;

				i++;
			}

			return indices;
		}

		inline bool check_extension_support(RHI_Device* rhi_device, const VkPhysicalDevice device)
		{
			uint32_t extensionCount;
			vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

			std::vector<VkExtensionProperties> available_extensions(extensionCount);
			vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, available_extensions.data());

			std::set<std::string> required_extensions(rhi_device->GetContext()->extensions_device.begin(), rhi_device->GetContext()->extensions_device.end());
			for (const auto& extension : available_extensions)
			{
				required_extensions.erase(extension.extensionName);
			}

			return required_extensions.empty();
		}

		inline bool choose(RHI_Device* rhi_device, void* window_handle, const std::vector<VkPhysicalDevice>& physical_devices)
		{
			// Temporarily create a surface just to check compatibility
			VkSurfaceKHR surface_temp = nullptr;
			{
				VkWin32SurfaceCreateInfoKHR create_info = {};
				create_info.sType						= VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
				create_info.hwnd						= static_cast<HWND>(window_handle);
				create_info.hinstance					= GetModuleHandle(nullptr);

				auto result = vkCreateWin32SurfaceKHR(rhi_device->GetContext()->instance, &create_info, nullptr, &surface_temp);
				if (result != VK_SUCCESS)
				{
					LOGF_ERROR("Failed to create Win32 surface, %s.", vulkan_helper::result_to_string(result));
					return false;
				}
			}

			for (const auto& device : physical_devices)
			{		
				bool extensions_supported	= check_extension_support(rhi_device, device);
				auto _indices				= get_family_indices(rhi_device, surface_temp, device);

				bool is_suitable = _indices.IsComplete() && extensions_supported;
				if (is_suitable)
				{			
					rhi_device->GetContext()->device_physical	= device;
					rhi_device->GetContext()->indices			= _indices;
					return true;
				}
			}

			// Destroy the surface
			vkDestroySurfaceKHR(rhi_device->GetContext()->instance, surface_temp, nullptr);

			return false;
		}
	}

	namespace command_list
	{
		inline bool create_command_buffer(RHI_Context* context, void*& cmd_buffer, void*& cmd_pool, VkCommandBufferLevel level)
		{
			VkCommandBufferAllocateInfo allocInfo = {};
			allocInfo.sType					= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			allocInfo.commandPool			= static_cast<VkCommandPool>(cmd_pool);
			allocInfo.level					= level;
			allocInfo.commandBufferCount	= 1;

			auto cmd_buffer_temp = reinterpret_cast<VkCommandBuffer*>(&cmd_buffer);
			auto result = vkAllocateCommandBuffers(context->device, &allocInfo, cmd_buffer_temp);
			if (result != VK_SUCCESS)
			{
				LOGF_ERROR("Failed to allocate command buffer, %s.", result_to_string(result));
				return false;
			}
			return true;
		}

		inline bool create_command_pool(RHI_Context* context, void*& cmd_pool)
		{
			VkCommandPoolCreateInfo cmd_pool_info	= {};
			cmd_pool_info.sType						= VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			cmd_pool_info.queueFamilyIndex			= context->indices.graphics_family.value();
			cmd_pool_info.flags						= VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

			auto cmd_pool_temp = reinterpret_cast<VkCommandPool*>(&cmd_pool);
			auto result = vkCreateCommandPool(context->device, &cmd_pool_info, nullptr, cmd_pool_temp);
			if (result != VK_SUCCESS)
			{
				LOGF_ERROR("Failed to create command pool, %s.", result_to_string(result));
				return false;
			}
			return true;
		}
	}

	namespace semaphore
	{
		inline void* create(RHI_Device* rhi_device)
		{
			VkSemaphoreCreateInfo semaphore_info = {};
			semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

			VkSemaphore semaphore_out;
			auto result = vkCreateSemaphore(rhi_device->GetContext()->device, &semaphore_info, nullptr, &semaphore_out);
			if (result != VK_SUCCESS)
			{
				LOGF_ERROR("Failed to create semaphore, %s.", result_to_string(result));
				return nullptr;
			}

			return static_cast<void*>(semaphore_out);
		}

		inline void destroy(RHI_Device* rhi_device, void*& semaphore_in)
		{
			if (!semaphore_in)
				return;

			vkDestroySemaphore(rhi_device->GetContext()->device, static_cast<VkSemaphore>(semaphore_in), nullptr);
			semaphore_in = nullptr;
		}
	}

	namespace fence
	{
		inline void* create(RHI_Device* rhi_device)
		{
			VkFenceCreateInfo fence_info	= {};
			fence_info.sType				= VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	
			VkFence fence_out;
			auto result = vkCreateFence(rhi_device->GetContext()->device, &fence_info, nullptr, &fence_out);
			if (result != VK_SUCCESS)
			{
				LOGF_ERROR("Failed to create semaphore, %s", result_to_string(result));
				return nullptr;
			}

			return static_cast<void*>(fence_out);
		}

		inline void destroy(RHI_Device* rhi_device, void*& fence_in)
		{
			if (!fence_in)
				return;

			if (fence_in) { vkDestroyFence(rhi_device->GetContext()->device, static_cast<VkFence>(fence_in), nullptr); }
			fence_in = nullptr;
		}

		inline void wait(RHI_Device* rhi_device, void*& fence_in)
		{
			auto fence_temp = reinterpret_cast<VkFence*>(&fence_in);
			auto result = vkWaitForFences(rhi_device->GetContext()->device, 1, fence_temp, true, 0xFFFFFFFFFFFFFFFF);
			SPARTAN_ASSERT(result == VK_SUCCESS);			
		}

		inline void reset(RHI_Device* rhi_device, void*& fence_in)
		{
			auto fence_temp = reinterpret_cast<VkFence*>(&fence_in);
			auto result = vkResetFences(rhi_device->GetContext()->device, 1, fence_temp);
			SPARTAN_ASSERT(result == VK_SUCCESS);
		}
	}
}
#endif