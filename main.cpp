#include <stdio.h>

#include "astcenc.h"

//#define STB_IMAGE_IMPLEMENTATION
//#include "ThirdParty/stb_image.h"
//
//#define STB_IMAGE_WRITE_IMPLEMENTATION
//#include "ThirdParty/stb_image_write.h"
#include <thread>
#include <vector>
#include <astcenccli_internal.h>
#include <astcenccli_toplevel.cpp>

int CompressASTC(char** argv);

int main(int argc, char** argv)
{
	return CompressASTC(argv);
}

int CompressASTC(char** argv)
{
	static const unsigned int thread_count = 20;
	static const unsigned int block_x = 4;
	static const unsigned int block_y = 4;
	static const unsigned int block_z = 1;
	static const astcenc_profile profile = ASTCENC_PRF_LDR;
	static const float quality = ASTCENC_PRE_MEDIUM;
	static const astcenc_swizzle swizzle{
		ASTCENC_SWZ_R, ASTCENC_SWZ_G, ASTCENC_SWZ_B, ASTCENC_SWZ_A
	};

	astcenc_config config;
	config.block_x = block_x;
	config.block_y = block_y;
	config.block_z = block_z;
	config.profile = profile;

	astcenc_error status;
	status = astcenc_config_init(profile, block_x, block_y, block_z, quality, 0, &config);

	astcenc_context* context;
	status = astcenc_context_alloc(&config, thread_count, &context);

	//uint8_t* image_data = (uint8_t*)stbi_load(argv[1], &image_x, &image_y, &image_c, 4);
	bool isHdr;
	unsigned int componentCount;
	astcenc_image* image_uncomp_in = load_png_with_wuffs(argv[1], false, isHdr, componentCount);

	double image_size = static_cast<double>(image_uncomp_in->dim_x) *
		static_cast<double>(image_uncomp_in->dim_y) *
		static_cast<double>(image_uncomp_in->dim_z);


	unsigned int blocks_x = (image_uncomp_in->dim_x + block_x - 1) / block_x;
	unsigned int blocks_y = (image_uncomp_in->dim_y + block_y - 1) / block_y;
	size_t buffer_size = blocks_x * blocks_y * 16;
	uint8_t* buffer = new uint8_t[buffer_size];


	//多线程
	compression_workload work;
	work.context = context;
	work.image = image_uncomp_in;
	work.swizzle = { ASTCENC_SWZ_R, ASTCENC_SWZ_G, ASTCENC_SWZ_B, ASTCENC_SWZ_A };
	work.data_out = buffer;
	work.data_len = buffer_size;
	work.error = ASTCENC_SUCCESS;

	launch_threads("Compression", thread_count, compression_workload_runner, &work);
	astcenc_compress_reset(context);


	//单线程，暂时不删除，可能用来调试yFlip，压缩mip链而不是单张等astc问题
	//status = astcenc_compress_image(context, image_uncomp_in, &swizzle, buffer, buffer_size, 0);
	//if (status != ASTCENC_SUCCESS)
	//{
	//	printf("ERROR: Codec compress failed: %s\n", astcenc_get_error_string(status));
	//	return 1;
	//}

	astc_compressed_image image_comp{};
	image_comp.block_x = config.block_x;
	image_comp.block_y = config.block_y;
	image_comp.block_z = config.block_z;
	image_comp.dim_x = image_uncomp_in->dim_x;
	image_comp.dim_y = image_uncomp_in->dim_y;
	image_comp.dim_z = image_uncomp_in->dim_z;
	image_comp.data = buffer;
	image_comp.data_len = buffer_size;
	int error = store_cimage(image_comp, argv[2]);
	if (error)
	{
		print_error("ERROR: Failed to store compressed image\n");
		return 1;
	}

	free_image(image_uncomp_in);
	//stbi_image_free(image_data);
	astcenc_context_free(context);
	delete[] image_comp.data;

	return 0;
}