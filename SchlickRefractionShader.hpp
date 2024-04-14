#pragma once
#include "Shader.hpp"
#include "GeomUtil.hpp"

class SchlickRefractionShader : public Shader
{
private:
	float indexOfRefraction_;
public:
	SchlickRefractionShader(float indexOfRefraction)
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

		float R_0;
		if (hitInfo.inDirection.dot(hitInfo.normal) > 0.f) {
			R_0 = (1.f - indexOfRefraction_) / (1.f + indexOfRefraction_);
		}
		else {
			R_0 = (indexOfRefraction_ - 1.f) / (indexOfRefraction_ + 1.f);
		}
		R_0 = R_0 * R_0;

		float cosAngle = std::fabsf(hitInfo.inDirection.dot(hitInfo.normal));
		float reflectCoefft = R_0 + (1.f - R_0) * powf((1 - cosAngle), 5.f);



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
			color += (1.f - reflectCoefft) * refractionHit.shader->getColor(
				refractionHit, scene, 
				lights, ambientLight, 
				currBounceCount+1, maxBounces);
		}

		Ray reflectedRay;
		reflectedRay.direction = reflect(hitInfo.inDirection, hitInfo.normal).normalized();
		reflectedRay.origin = hitInfo.location;

		if (hitInfo.inDirection.dot(hitInfo.normal) > 0.f) {
			refractedRay.origin -= 1e-4 * hitInfo.normal;
		}
		else {
			refractedRay.origin += 1e-4 * hitInfo.normal;
		}
		HitInfo reflectionHit;
		if (scene->intersect(reflectedRay, 1e-4f, 1e4f, reflectionHit, VISIBLE_BITMASK)) {
			color += reflectCoefft * reflectionHit.shader->getColor(
				reflectionHit, scene, 
				lights, ambientLight, 
				currBounceCount+1, maxBounces);
		}


		return color;
	}
};
