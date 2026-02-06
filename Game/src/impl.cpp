#include <pch.h>

/*
	NOTE:
	Put implementation defines from any third party libraries here.
*/

// Font loading.

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

// Texure and model loading:

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image.h>

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

//#define TINYGLTF_IMPLEMENTATION
//// stb image includes are in tiny_gltf.h
//#include <tiny_gltf.h>