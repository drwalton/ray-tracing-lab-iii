#include <Eigen/Dense>
#include <tgaimage.h>
#include <json/json.hpp>
#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include "BVHNode.hpp"
#include "Sphere.hpp"
#include "Plane.hpp"
#include "Triangle.hpp"
#include "Scene.hpp"
#include "Camera.hpp"
#include "PointLight.hpp"
#include "DirectionalLight.hpp"
#include "LambertianShader.hpp"
#include "TexturedLambertianShader.hpp"
#include "PhongShader.hpp"
#include "RefractionShader.hpp"
#include "SchlickRefractionShader.hpp"
#include "MirrorShader.hpp"
#include "TexCoordTestShader.hpp"
#include "Model.hpp"

/// <summary>
/// Load a JSON config file using the nlohmann library.
/// </summary>
nlohmann::json loadConfig(const std::string& filename)
{
	std::ifstream configStream(filename);
	nlohmann::json config = nlohmann::json::parse(configStream);
	return config;
}

/// <summary>
/// Load an Eigen Vector3f from a config file.
/// Call as for example loadVec3FromConfig(config["myVector3"]);
/// </summary>
Eigen::Vector3f loadVec3FromConfig(const nlohmann::json& config)
{
	return Eigen::Vector3f(config[0], config[1], config[2]);
}

int main(int argc, char* argv[]) {

	// *** Load the config file ***
	auto config = loadConfig("../config/config.json");

	int pixHeight = config["pixHeight"], pixWidth = config["pixWidth"];

	// Color that will be drawn where no objects are present.
	TGAColor clearColor(
		config["clearColor"][0], config["clearColor"][1], 
		config["clearColor"][2], config["clearColor"][3]);

	// *** Set up camera and output image ***
	Camera cam(
		loadVec3FromConfig(config["cameraPos"]),
		loadVec3FromConfig(config["cameraForward"]),
		loadVec3FromConfig(config["cameraUp"]),
		pixWidth, pixHeight,
		config["cameraFov"]);

	TGAImage outImage(pixWidth, pixHeight, TGAImage::RGB);

	Eigen::Vector3f
		red(1.f, 0.f, 0.f),
		blue(0.f, 0.f, 1.f),
		aqua(0.f, .8f, .8f),
		lavender(178.f / 255.f, 164.f / 255.f, 212.f / 255.f);

	// *** Load shaders and textures ***
	TGAImage spotTexture;
	spotTexture.read_tga_file("../models/spot.tga");
	LambertianShader redLambertianShader(red);
	PhongShader bluePlasticShader(blue, Eigen::Vector3f(1.f, 1.f, 1.f), 100.f);
	LambertianShader aquaLambertianShader(aqua);
	LambertianShader lavenderLambertianShader(lavender);
	TexturedLambertianShader spotShader(&spotTexture);
	//RefractionShader refractionShader(1.1f);
	SchlickRefractionShader refractionShader(1.1f);
	MirrorShader mirrorShader;
	TexCoordTestShader texCoordTestShader;

	// *** Set up scene ***
	Scene scene;

	// Set up some planes to surround our scene.
	scene.renderables.push_back(std::make_shared<Plane>(&aquaLambertianShader, Eigen::Vector3f(0.f, 0.f, -1.f)));
	scene.renderables.back()->modelToWorld(makeTranslationMatrix(Eigen::Vector3f(0.f, 0.f, 3.f)));

	scene.renderables.push_back(std::make_shared<Plane>(&lavenderLambertianShader, Eigen::Vector3f(0.f, 1.f, 0.f)));
	scene.renderables.back()->modelToWorld(makeTranslationMatrix(Eigen::Vector3f(0.f, -3.f, 0.f)));

	scene.renderables.push_back(std::make_shared<Plane>(&aquaLambertianShader, Eigen::Vector3f(0.f, 0.f, 1.f), VISIBLE_BITMASK));
	scene.renderables.back()->modelToWorld(makeTranslationMatrix(Eigen::Vector3f(0.f, 0.f, -6.f)));

	scene.renderables.push_back(std::make_shared<Plane>(&aquaLambertianShader, Eigen::Vector3f(0.f, 0.f, 1.f), VISIBLE_BITMASK));
	scene.renderables.back()->modelToWorld(makeTranslationMatrix(Eigen::Vector3f(0.f, 0.f, -6.f)));


	std::vector<std::shared_ptr<Renderable>> spheres;

	float sphereSpacing = 0.5f;
	float sphereRadius = 0.2f;

	int sphereCountXY = 3;

	for (int x = -sphereCountXY; x <= sphereCountXY; ++x) {
		for (int y = -sphereCountXY; y <= sphereCountXY; ++y) {
			for (int z = -1; z <= 1; ++z) {
				spheres.push_back(std::make_shared<Sphere>(&mirrorShader, sphereRadius));
				spheres.back()->modelToWorld(makeTranslationMatrix(Eigen::Vector3f(x*sphereSpacing, y*sphereSpacing, z*sphereSpacing)));
			}
		}
	}

	// This version of the code doesn't use a BVH, and just adds all the spheres to the scene.
	scene.renderables.insert(scene.renderables.end(), spheres.begin(), spheres.end());

	// This version adds the spheres to a BVH (this will only work once you've implemented it!
	//scene.renderables.push_back(std::make_shared<BVHNode>(spheres, 5));

	// Optional code: here's how to add the spot mesh to the scene, using a BVH
	// Try enabling this and comparing it to the non-BVH version below!
	//Model spotModel("../models/spot.obj");
	//scene.renderables.push_back(std::make_shared<BVHNode>(spotModel, &spotShader, 4, rotateY(M_PI / 4.0f)));

	// Here's how to add the mesh without using the BVH.
	// Try comparing performance to the BVH version above.
	//Model spotModel("../models/spot.obj");
	//scene.renderables.push_back(std::make_shared<Mesh>(&spotShader, &spotModel));
	//scene.renderables.back()->modelToWorld(rotateY(M_PI / 4.0f));

	// *** Add lights to scene ***
	Eigen::Vector3f ambientLight(.1f, .1f, .1f);

	std::vector<std::unique_ptr<Light>> lightSources;
	lightSources.push_back(std::make_unique<PointLight>(Eigen::Vector3f(-1.f, 3.f, -1.f), 3.f * Eigen::Vector3f(1.f, 1.f, 1.f)));
	lightSources.push_back(std::make_unique<DirectionalLight>(Eigen::Vector3f(0.f, -1.f, 1.f), .5f * Eigen::Vector3f(1.f, 1.f, 1.f)));

	// *** Render the scene ***

	// Shuffling the scanline order gets better CPU usage between threads
	// when some lines take longer to render than others.
	std::vector<unsigned int> scanlines(pixHeight);
	for (int i = 0; i < pixHeight; ++i) scanlines[i] = i;

	if (config["shuffleScanlines"]) {
		std::random_device rd;
		std::mt19937 g(rd());
		std::shuffle(scanlines.begin(), scanlines.end(), g);
	}

	auto startTime = std::chrono::steady_clock::now();

	Ray ray = cam.getRay(531, 325);
	HitInfo hitInfo;
	scene.intersect(ray, 1e-6f, 1e6f, hitInfo, VISIBLE_BITMASK);
	float x = hitInfo.hitT;


	#pragma omp parallel for
	for (int y = 0; y < pixHeight; ++y) {
		for (int x = 0; x < pixWidth; ++x) {
			Ray ray = cam.getRay(x, scanlines[y]);
			HitInfo hitInfo;
			if (scene.intersect(ray, 1e-6f, 1e6f, hitInfo, VISIBLE_BITMASK)) {
				Eigen::Vector3f color = hitInfo.shader->getColor(
					hitInfo, &scene,
					lightSources, ambientLight,
					0, config["maxBounces"]);

				color.x() = std::min(color.x(), 1.f);
				color.y() = std::min(color.y(), 1.f);
				color.z() = std::min(color.z(), 1.f);

				TGAColor tgaColor(color.x() * 255, color.y() * 255, color.z() * 255, 255);
				outImage.set(x, scanlines[y], tgaColor);
			}
			else
				outImage.set(x, scanlines[y], clearColor);
		}
		if (omp_get_thread_num() == omp_get_num_threads()-1) {
			std::clog << "\rScanlines remaining: " << (pixHeight - y) << ' ' << std::flush;
		}

	}

	auto renderTime = std::chrono::steady_clock::now() - startTime;

	std::cout << "Render duration " << std::chrono::duration_cast<std::chrono::milliseconds>(renderTime).count() * 1e-3f << " seconds." << std::endl;

	// *** Save the output image ***
	outImage.flip_vertically();
	std::string outputFilename = config["outputFilename"];
	outImage.write_tga_file(outputFilename.c_str());

	return 0;
}
