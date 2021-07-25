#include "AnimInstance_HIK.h"
#include "ik_logger.h"

UAnimInstance_HIK::UAnimInstance_HIK()
	: m_hConf(H_INVALID)
	, m_match(NULL)
	, m_nMatches(0)
	, m_bvh2fbxWorld{
		{1, 0, 0},
		{0, 1, 0},
		{0, 0, 1}
	}
	, m_scales{NULL, NULL}
	, m_nScales{0, 0}
	, m_targetnames{ NULL, NULL }
	, m_nTargets{0, 0}
	, m_filenames{NULL, NULL}

{}

void UAnimInstance_HIK::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	if (VALID_HANDLE(m_hConf))
		unload_conf(m_hConf);

	FString rootDir = FPaths::ProjectDir();
	FString drvpath_full = rootDir + GetFileConfName();
	m_hConf = load_conf(*drvpath_full);
	LOGIK(TCHAR_TO_ANSI(*drvpath_full));

	bool conf_loaded = VALID_HANDLE(m_hConf);
	LOGIKVar(LogInfoBool, conf_loaded);

	HCONFMOPIPE hConfMoPipe = init_conf_mopipe(m_hConf);
	bool valid_fkrc = (VALID_HANDLE(hConfMoPipe));
	LOGIKVar(LogInfoBool, valid_fkrc);
	bool ok = conf_loaded && valid_fkrc;
	if (ok)
	{
		bool mtx_loaded = get_mopipe_mtx(hConfMoPipe, m_bvh2fbxWorld);
		m_nMatches = load_mopipe_pairs(hConfMoPipe, &m_match);
		bool filenames_loaded = load_file_names(hConfMoPipe, m_filenames);
		bool scale_loaded = load_scale(hConfMoPipe, m_scales, m_nScales);
		bool match_loaded = (m_nMatches > 0);
		bool targets_loaded = load_endeff_names(hConfMoPipe, m_targetnames, m_nTargets);
		ok = ( filenames_loaded
			&& scale_loaded
			&& mtx_loaded
			&& match_loaded
			&& targets_loaded);
		LOGIKVar(LogInfoBool, filenames_loaded);
		LOGIKVar(LogInfoBool, scale_loaded);
		LOGIKVar(LogInfoBool, mtx_loaded);
		LOGIKVar(LogInfoBool, match_loaded);
		LOGIKVar(LogInfoBool, targets_loaded);
	}
	uninit_conf_mopipe(hConfMoPipe);
}

void UAnimInstance_HIK::NativeUninitializeAnimation()
{
	free_scale(m_scales, m_nScales);
	m_scales[0] = NULL; m_scales[1] = NULL;
	m_nScales[0] = 0; m_nScales[1] = 0;
	free_mopipe_pairs(m_match, m_nMatches);
	m_match = NULL; m_nMatches = 0;
	free_endeff_names(m_targetnames, m_nTargets);
	m_targetnames[0] = NULL; m_targetnames[1] = NULL;
	m_nTargets[0] = 0; m_nTargets[1] = 0;

	free_file_names(m_filenames);
	m_filenames[0] = NULL; m_filenames[1] = NULL;

	if (VALID_HANDLE(m_hConf))
		unload_conf(m_hConf);
	m_hConf = H_INVALID;

	Super::NativeUninitializeAnimation();
}


