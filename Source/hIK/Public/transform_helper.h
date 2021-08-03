#pragma once

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

template<typename LAMaccessEnter, typename LAMaccessLeave>
inline void TraverseDFS(HBODY root, LAMaccessEnter OnEnterBody, LAMaccessLeave OnLeaveBody)
{
	check(VALID_HANDLE(root));
	typedef struct _EDGE
	{
		HBODY body_this;
		HBODY body_child;
	} EDGE;
	std::stack<EDGE> stkDFS;
	stkDFS.push({root, get_first_child_body(root)});
	//printArtName(body_name_w(root), 0);
	OnEnterBody(root);
	while (!stkDFS.empty())
	{
		EDGE &edge = stkDFS.top();
		int n_indent = stkDFS.size();
		if (!VALID_HANDLE(edge.body_child))
		{
			stkDFS.pop();
			OnLeaveBody(edge.body_this);
		}
		else
		{
			//printArtName(body_name_w(edge.body_child), n_indent);
			OnEnterBody(edge.body_child);
			HBODY body_grandchild = get_first_child_body(edge.body_child);
			HBODY body_nextchild = get_next_sibling_body(edge.body_child);
			stkDFS.push({edge.body_child, body_grandchild});
			edge.body_child = body_nextchild;
		}
	}
}
