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


//int main(int argc, char** argv)
//{
//	float astcenc_profile = ASTCENC_PRE_MEDIUM;
//	int blockDim = 6;//6x6
//	bool isHDR = false;
//
//	astcenc_config astcenc_cfg;
//	if (astcenc_config_init(isHDR ? ASTCENC_PRF_HDR : ASTCENC_PRF_LDR,
//		blockDim, blockDim, 1,
//		astcenc_profile,
//		0, &astcenc_cfg)
//		!= ASTCENC_SUCCESS)
//		return;
//	//Note: May be noticably more efficient to reuse a context, as per the astcenc documentation.
//	astcenc_context* pAstcencContext = nullptr;
//	unsigned int numThreads = 1;
//	bool mt = false;
//	if (mt)
//	{
//		numThreads = std::thread::hardware_concurrency();
//		if (numThreads == 0) numThreads = 2;
//	}
//	if (astcenc_context_alloc(&astcenc_cfg, numThreads, &pAstcencContext)
//		!= ASTCENC_SUCCESS)
//		return;
//	astcenc_compress_reset(pAstcencContext);
//	astcenc_image image_in = {};
//	void* dataSlices[1] = { pRGBA32Buf }; //z size = 1
//	image_in.data = &dataSlices[0];
//	image_in.data_type = ASTCENC_TYPE_U8;
//	image_in.dim_x = pTex->m_Width;
//	image_in.dim_y = pTex->m_Height;
//	image_in.dim_z = 1;
//	astcenc_swizzle swizzle = {};
//	swizzle.r = ASTCENC_SWZ_R;
//	swizzle.g = ASTCENC_SWZ_G;
//	swizzle.b = ASTCENC_SWZ_B;
//	swizzle.a = ASTCENC_SWZ_A;
//	std::vector<std::jthread> threads(numThreads - 1);
//	for (unsigned int i = 1; i < numThreads; ++i)
//	{
//		threads[i - 1] = std::jthread([pAstcencContext, &image_in, &swizzle, pOutBuf, outputSize, i]()
//			{
//				astcenc_compress_image(pAstcencContext, &image_in, &swizzle, (uint8_t*)pOutBuf, outputSize, i);
//			});
//	}
//	auto result = astcenc_compress_image(pAstcencContext, &image_in, &swizzle, (uint8_t*)pOutBuf, outputSize, 0);
//	threads.clear();
//	astcenc_context_free(pAstcencContext);
//	if (result != ASTCENC_SUCCESS)
//		return;
//	outputSize = GetCompressedTextureDataSize((int)pTex->m_Width, (int)pTex->m_Height, (TextureFormat)pTex->m_TextureFormat);
//}


int main(int argc, char** argv)
{
	static const unsigned int thread_count = 1;
	static const unsigned int block_x = 6;
	static const unsigned int block_y = 6;
	static const unsigned int block_z = 1;
	static const astcenc_profile profile = ASTCENC_PRF_LDR;
	static const float quality = ASTCENC_PRE_MEDIUM;
	static const astcenc_swizzle swizzle{
		ASTCENC_SWZ_R, ASTCENC_SWZ_G, ASTCENC_SWZ_B, ASTCENC_SWZ_A
	};

	uint8_t* image_data = nullptr;// (uint8_t*)stbi_load(argv[1], &image_x, &image_y, &image_c, 4);
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

	astcenc_config config;
	config.block_x = block_x;
	config.block_y = block_y;
	config.block_z = block_z;
	config.profile = profile;

	astcenc_error status;
	status = astcenc_config_init(profile, block_x, block_y, block_z, quality, 0, &config);

	astcenc_context* context;
	status = astcenc_context_alloc(&config, thread_count, &context);

	status = astcenc_compress_image(context, image_uncomp_in, &swizzle, buffer, buffer_size, 0);
	if (status != ASTCENC_SUCCESS)
	{
		printf("ERROR: Codec compress failed: %s\n", astcenc_get_error_string(status));
		return 1;
	}

	//stbi_write_png(argv[2], image_x, image_y, 4, image_data, 4 * image_x);


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