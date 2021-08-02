#include "articulated_body.h"
#include "Math/TransformNonVectorized.h"

FORCEINLINE void Convert(const FTransform& tm_s, _TRANSFORM& tm_t) 
{
	const auto& s_s = tm_s.GetScale3D();
	const auto& r_s = tm_s.GetRotation();
	const auto& tt_s = tm_s.GetTranslation();

	tm_t.s.x = s_s.X;
	tm_t.s.y = s_s.Y;
	tm_t.s.z = s_s.Z;

	tm_t.r.w = r_s.W;
	tm_t.r.x = r_s.X;
	tm_t.r.y = r_s.Y;
	tm_t.r.z = r_s.Z;

	tm_t.tt.x = tt_s.X;
	tm_t.tt.y = tt_s.Y;
	tm_t.tt.z = tt_s.Z;
}

FORCEINLINE void Convert(const _TRANSFORM& tm_s, FTransform& tm_t) 
{
	const auto& s_s = tm_s.s;
	const auto& r_s = tm_s.r;
	const auto& tt_s = tm_s.tt;

	tm_t.SetLocation(FVector(tt_s.x, tt_s.y, tt_s.z));
	tm_t.SetRotation(FQuat(r_s.x, r_s.y, r_s.z, r_s.w));
	tm_t.SetScale3D(FVector(s_s.x, s_s.y, s_s.z));
}
