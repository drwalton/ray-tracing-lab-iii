#pragma once
#include "Shader.hpp"
#include "GeomUtil.hpp"

class RefractionShader : public Shader
{
private:
	float indexOfRefraction_;
public:
	RefractionShader(float indexOfRefraction)
		:indexOfRefraction_(indexOfRefraction)
	{}

	virtual Eigen::Vector3f getColor(const HitInfo& hitInfo, 
		const Renderable* scene, 
		const std::vector<std::unique_ptr<Light>>& lights,
		const Eigen::Vector3f& ambientLight,
		int currBounceCount,
		const int maxBounces) const
	{
		if (currBounceCount >= maxBounces) return Eigen::Vector3f::Zero();

		Ray refractedRay;
		refractedRay.direction = refract(hitInfo.inDirection, hitInfo.normal, indexOfRefraction_).normalized();
		refractedRay.origin = hitInfo.location;

		if (hitInfo.inDirection.dot(hitInfo.normal) > 0.f) {
			refractedRay.origin += 1e-4 * hitInfo.normal;
		}
		else {
			refractedRay.origin -= 1e-4 * hitInfo.normal;
		}

		Eigen::Vector3f color = Eigen::Vector3f::Zero();

		HitInfo refractionHit;
		if (scene->intersect(refractedRay, 1e-4f, 1e4f, refractionHit, VISIBLE_BITMASK)) {
			color = refractionHit.shader->getColor(
				refractionHit, scene, 
				lights, ambientLight, 
				currBounceCount+1, maxBounces);
		}

		return color;
	}
};
