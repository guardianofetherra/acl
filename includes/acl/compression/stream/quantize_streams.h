#pragma once

////////////////////////////////////////////////////////////////////////////////
// The MIT License (MIT)
//
// Copyright (c) 2017 Nicholas Frechette & Animation Compression Library contributors
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
////////////////////////////////////////////////////////////////////////////////

#include "acl/core/memory.h"
#include "acl/core/error.h"
#include "acl/math/quat_64.h"
#include "acl/math/quat_packing.h"
#include "acl/math/vector4_64.h"
#include "acl/math/vector4_packing.h"
#include "acl/compression/stream/track_stream.h"

#include <stdint.h>

namespace acl
{
	inline void quantize_rotation_streams(Allocator& allocator, BoneStreams* bone_streams, uint16_t num_bones, RotationFormat8 rotation_format)
	{
		if (num_bones == 0)
			return;

		for (uint16_t bone_index = 0; bone_index < num_bones; ++bone_index)
		{
			BoneStreams& bone_stream = bone_streams[bone_index];

			// We expect all our samples to have the same width of sizeof(Vector4_64)
			ACL_ENSURE(bone_stream.rotations.get_sample_size() == sizeof(Vector4_64), "Unexpected rotation sample size. %u != %u", bone_stream.rotations.get_sample_size(), sizeof(Vector4_64));

			uint32_t num_samples = bone_stream.rotations.get_num_samples();
			TrackStream quantized_stream(allocator, num_samples, get_packed_rotation_size(rotation_format));

			Vector4_64 rotation_min = vector_set(1e10);
			Vector4_64 rotation_max = vector_set(-1e10);

			for (uint32_t sample_index = 0; sample_index < num_samples; ++sample_index)
			{
				Quat_64 rotation = bone_stream.rotations.get_sample<Quat_64>(sample_index);
				uint8_t* quantized_ptr = quantized_stream.get_sample_ptr(sample_index);

				pack_rotation(quat_cast(rotation), rotation_format, quantized_ptr);
				rotation = quat_cast(unpack_rotation(rotation_format, quantized_ptr));

				rotation_min = vector_min(rotation_min, quat_to_vector(rotation));
				rotation_max = vector_max(rotation_max, quat_to_vector(rotation));
			}

			bone_stream.rotations = std::move(quantized_stream);
			bone_stream.rotation_range = TrackStreamRange(rotation_min, rotation_max);
		}
	}

	inline void quantize_translation_streams(Allocator& allocator, BoneStreams* bone_streams, uint16_t num_bones, VectorFormat8 translation_format)
	{
		if (num_bones == 0)
			return;

		for (uint16_t bone_index = 0; bone_index < num_bones; ++bone_index)
		{
			BoneStreams& bone_stream = bone_streams[bone_index];

			// We expect all our samples to have the same width of sizeof(Vector4_64)
			ACL_ENSURE(bone_stream.translations.get_sample_size() == sizeof(Vector4_64), "Unexpected translation sample size. %u != %u", bone_stream.translations.get_sample_size(), sizeof(Vector4_64));

			uint32_t num_samples = bone_stream.translations.get_num_samples();
			TrackStream quantized_stream(allocator, num_samples, get_packed_vector_size(translation_format));

			Vector4_64 translation_min = vector_set(1e10);
			Vector4_64 translation_max = vector_set(-1e10);

			for (uint32_t sample_index = 0; sample_index < num_samples; ++sample_index)
			{
				Vector4_64 translation = bone_stream.translations.get_sample<Vector4_64>(sample_index);
				uint8_t* quantized_ptr = quantized_stream.get_sample_ptr(sample_index);

				pack_vector(vector_cast(translation), translation_format, quantized_ptr);
				translation = vector_cast(unpack_vector(translation_format, quantized_ptr));

				translation_min = vector_min(translation_min, translation);
				translation_max = vector_max(translation_max, translation);
			}

			bone_stream.translations = std::move(quantized_stream);
			bone_stream.translation_range = TrackStreamRange(translation_min, translation_max);
		}
	}
}