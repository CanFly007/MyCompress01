﻿#include <stdio.h>

#include "astcenc.h"

#define STB_IMAGE_IMPLEMENTATION
#include "ThirdParty/stb_image.h"
//#define STB_IMAGE_WRITE_IMPLEMENTATION
//#include "ThirdParty/stb_image_write.h"
#include <thread>
#include <vector>
#include <astcenccli_internal.h>
#include <astcenccli_toplevel.cpp>

#include <ispc_texcomp.h>

int CompressASTC(char** argv);
int CompressBC7(char** argv);
int CompressBC3(char** argv);
int CompressBC1(char** argv);


//BC7的Header
#include <stdint.h>
#include <stdlib.h>
#define MAKEFOURCC(ch0, ch1, ch2, ch3) \
                ((uint32_t)(uint8_t)(ch0) | ((uint32_t)(uint8_t)(ch1) << 8) | \
                ((uint32_t)(uint8_t)(ch2) << 16) | ((uint32_t)(uint8_t)(ch3) << 24 ))

#define DDS_MAGIC 0x20534444  // "DDS "

#define DDSD_CAPS 0x1
#define DDSD_HEIGHT 0x2
#define DDSD_WIDTH 0x4
#define DDSD_PIXELFORMAT 0x1000
#define DDSD_LINEARSIZE 0x80000

#define DDPF_FOURCC 0x4

#define DDSCAPS_TEXTURE 0x1000

#define DXGI_FORMAT_BC7_UNORM 98
#define D3D10_RESOURCE_DIMENSION_TEXTURE2D 3

typedef struct {
	uint32_t       dwSize;
	uint32_t       dwFlags;
	uint32_t       dwHeight;
	uint32_t       dwWidth;
	uint32_t       dwPitchOrLinearSize;
	uint32_t       dwDepth;
	uint32_t       dwMipMapCount;
	uint32_t       dwReserved1[11];
	struct {
		uint32_t   dwSize;
		uint32_t   dwFlags;
		uint32_t   dwFourCC;
		uint32_t   dwRGBBitCount;
		uint32_t   dwRBitMask;
		uint32_t   dwGBitMask;
		uint32_t   dwBBitMask;
		uint32_t   dwABitMask;
	} ddpfPixelFormat;
	struct {
		uint32_t   dwCaps1;
		uint32_t   dwCaps2;
		uint32_t   dwDDSX;
		uint32_t   dwReserved;
	} ddsCaps;
	uint32_t       dwReserved2;
} DDS_HEADER;

typedef struct {
	uint32_t dxgiFormat;
	uint32_t resourceDimension;
	uint32_t miscFlag;
	uint32_t arraySize;
	uint32_t miscFlags2;
} DDS_HEADER_DXT10;


void write_dds(const char* filename, int width, int height, const void* data, size_t dataSize, int bcN) 
{
	FILE* file = fopen(filename, "wb");
	if (!file) {
		fprintf(stderr, "Failed to open output file\n");
		return;
	}

	uint32_t ddsMagic = DDS_MAGIC;
	fwrite(&ddsMagic, sizeof(uint32_t), 1, file);

	DDS_HEADER header = { 0 };
	header.dwSize = 124;
	header.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT | DDSD_LINEARSIZE;
	header.dwHeight = height;
	header.dwWidth = width;
	header.dwPitchOrLinearSize = dataSize;
	header.dwDepth = 0;
	header.dwMipMapCount = 0;
	header.ddpfPixelFormat.dwSize = 32;
	header.ddpfPixelFormat.dwFlags = DDPF_FOURCC;
	if (bcN == 7)
	{
		header.ddpfPixelFormat.dwFourCC = MAKEFOURCC('D', 'X', '1', '0');
	}
	else if (bcN == 3)
	{
		header.ddpfPixelFormat.dwFourCC = MAKEFOURCC('D', 'X', 'T', '5');
	}
	else if (bcN == 1)
	{
		header.ddpfPixelFormat.dwFourCC = MAKEFOURCC('D', 'X', 'T', '1');
	}
	header.ddsCaps.dwCaps1 = DDSCAPS_TEXTURE;
	fwrite(&header, sizeof(DDS_HEADER), 1, file);

	if (bcN == 7) {
		DDS_HEADER_DXT10 dxt10Header = {
			DXGI_FORMAT_BC7_UNORM,
			D3D10_RESOURCE_DIMENSION_TEXTURE2D,
			0,
			1,
			0
		};
		fwrite(&dxt10Header, sizeof(DDS_HEADER_DXT10), 1, file);
	}

	fwrite(data, 1, dataSize, file);
	fclose(file);
}


int main(int argc, char** argv)
{
	//return CompressASTC(argv);

	//return CompressBC7(argv);
	//return CompressBC3(argv);
	return CompressBC1(argv);
}

int CompressBC1(char** argv)
{
	int image_x, image_y, image_c;
	uint8_t* image_data = (uint8_t*)stbi_load(argv[1], &image_x, &image_y, &image_c, 4);

	rgba_surface surfaceInput;
	surfaceInput.ptr = image_data;
	surfaceInput.stride = image_x * 4;
	surfaceInput.width = image_x;
	surfaceInput.height = image_y;

	// Calculate buffer size for output, considering each block compresses to 16 bytes
	size_t num_blocks_wide = (image_x + 3) / 4;
	size_t num_blocks_high = (image_y + 3) / 4;
	//size_t output_buffer_size = num_blocks_wide * num_blocks_high * 16;
	size_t output_buffer_size = num_blocks_wide * num_blocks_high * 8;
	uint8_t* output_buffer = (uint8_t*)malloc(output_buffer_size);

	//为什么单线程也能这么快？内部是多线程？
	CompressBlocksBC1(&surfaceInput, output_buffer);

	write_dds(argv[2], image_x, image_y, output_buffer, output_buffer_size, 1);

	free(output_buffer);
	stbi_image_free(image_data);
	return 1;
}

int CompressBC3(char** argv)
{
	int image_x, image_y, image_c;
	uint8_t* image_data = (uint8_t*)stbi_load(argv[1], &image_x, &image_y, &image_c, 4);

	rgba_surface surfaceInput;
	surfaceInput.ptr = image_data;
	surfaceInput.stride = image_x * 4;
	surfaceInput.width = image_x;
	surfaceInput.height = image_y;

	// Calculate buffer size for output, considering each block compresses to 16 bytes
	size_t num_blocks_wide = (image_x + 3) / 4;
	size_t num_blocks_high = (image_y + 3) / 4;
	size_t output_buffer_size = num_blocks_wide * num_blocks_high * 16;
	uint8_t* output_buffer = (uint8_t*)malloc(output_buffer_size);

	//为什么单线程也能这么快？内部是多线程？
	CompressBlocksBC3(&surfaceInput, output_buffer);

	write_dds(argv[2], image_x, image_y, output_buffer, output_buffer_size, 3);

	free(output_buffer);
	stbi_image_free(image_data);
	return 1;
}

int CompressBC7(char** argv)
{
	int image_x, image_y, image_c;
	uint8_t* image_data = (uint8_t*)stbi_load(argv[1], &image_x, &image_y, &image_c, 4);

	bc7_enc_settings encodingSettings;
	GetProfile_basic(&encodingSettings);
	//GetProfile_alpha_basic(&encodingSettings);

	rgba_surface surfaceInput;
	surfaceInput.ptr = image_data;
	surfaceInput.stride = image_x * 4;
	surfaceInput.width = image_x;
	surfaceInput.height = image_y;

	// Calculate buffer size for output, considering each block compresses to 16 bytes
	size_t num_blocks_wide = (image_x + 3) / 4;
	size_t num_blocks_high = (image_y + 3) / 4;
	size_t output_buffer_size = num_blocks_wide * num_blocks_high * 16;
	uint8_t* output_buffer = (uint8_t*)malloc(output_buffer_size);

	//为什么单线程也能这么快？内部是多线程？
	CompressBlocksBC7(&surfaceInput, output_buffer, &encodingSettings);

	write_dds(argv[2], image_x, image_y, output_buffer, output_buffer_size, 7);

	free(output_buffer);
	stbi_image_free(image_data);
	return 1;
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