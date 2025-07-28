//Maya ASCII 2022 scene
//Name: fpsr_algorithms.ma
//Last modified: Mon, Jul 28, 2025 12:16:02 PM
//Codeset: 1252
requires maya "2022";
requires -nodeType "colorCondition" "lookdevKit" "1.0";
requires -nodeType "type" -nodeType "shellDeformer" -nodeType "vectorAdjust" -nodeType "typeExtrude"
		 -nodeType "displayPoints" "Type" "2.0a";
requires "stereoCamera" "10.0";
requires "mtoa" "4.2.1";
currentUnit -l centimeter -a degree -t film;
fileInfo "application" "maya";
fileInfo "product" "Maya 2022";
fileInfo "version" "2022";
fileInfo "cutIdentifier" "202102181415-29bfc1879c";
fileInfo "osv" "Windows 10 Home v2009 (Build: 19045)";
fileInfo "UUID" "A8B262B4-4164-21F1-917B-0CB60E96E019";
createNode transform -s -n "persp";
	rename -uid "FE533258-4330-76E4-00FD-0E976AE214D0";
	setAttr ".v" no;
	setAttr ".t" -type "double3" 9.7000568444616491 5.9570772969112706 13.312586156283754 ;
	setAttr ".r" -type "double3" -15.938352729600446 23.800000000000381 0 ;
createNode camera -s -n "perspShape" -p "persp";
	rename -uid "F4E20B68-4D53-3216-C0CD-93B79A910172";
	setAttr -k off ".v" no;
	setAttr ".fl" 34.999999999999993;
	setAttr ".coi" 17.089180578928229;
	setAttr ".imn" -type "string" "persp";
	setAttr ".den" -type "string" "persp_depth";
	setAttr ".man" -type "string" "persp_mask";
	setAttr ".hc" -type "string" "viewSet -p %camera";
	setAttr ".ai_translator" -type "string" "perspective";
createNode transform -s -n "top";
	rename -uid "0D9A0CA7-4140-62FC-B2DB-C599047888F8";
	setAttr ".v" no;
	setAttr ".t" -type "double3" 0 1000.1 0 ;
	setAttr ".r" -type "double3" -90 0 0 ;
createNode camera -s -n "topShape" -p "top";
	rename -uid "F2E255B9-4CD6-CC79-A330-929EA5238930";
	setAttr -k off ".v" no;
	setAttr ".rnd" no;
	setAttr ".coi" 1000.1;
	setAttr ".ow" 30;
	setAttr ".imn" -type "string" "top";
	setAttr ".den" -type "string" "top_depth";
	setAttr ".man" -type "string" "top_mask";
	setAttr ".hc" -type "string" "viewSet -t %camera";
	setAttr ".o" yes;
	setAttr ".ai_translator" -type "string" "orthographic";
createNode transform -s -n "front";
	rename -uid "6102AFA6-4E91-C6B4-7FD4-1BAE1D040E0D";
	setAttr ".v" no;
	setAttr ".t" -type "double3" 0 0 1000.1 ;
createNode camera -s -n "frontShape" -p "front";
	rename -uid "8BCD6FAB-4CA2-44AC-302F-78AACA4FC0EA";
	setAttr -k off ".v" no;
	setAttr ".rnd" no;
	setAttr ".coi" 1000.1;
	setAttr ".ow" 30;
	setAttr ".imn" -type "string" "front";
	setAttr ".den" -type "string" "front_depth";
	setAttr ".man" -type "string" "front_mask";
	setAttr ".hc" -type "string" "viewSet -f %camera";
	setAttr ".o" yes;
	setAttr ".ai_translator" -type "string" "orthographic";
createNode transform -s -n "side";
	rename -uid "7FC000E2-41E6-5EE1-E138-5FAC0C665F10";
	setAttr ".v" no;
	setAttr ".t" -type "double3" 1000.1 0 0 ;
	setAttr ".r" -type "double3" 0 90 0 ;
createNode camera -s -n "sideShape" -p "side";
	rename -uid "0F12AB8B-4A83-8A81-0348-16915CDDA5EA";
	setAttr -k off ".v" no;
	setAttr ".rnd" no;
	setAttr ".coi" 1000.1;
	setAttr ".ow" 30;
	setAttr ".imn" -type "string" "side";
	setAttr ".den" -type "string" "side_depth";
	setAttr ".man" -type "string" "side_mask";
	setAttr ".hc" -type "string" "viewSet -s %camera";
	setAttr ".o" yes;
	setAttr ".ai_translator" -type "string" "orthographic";
createNode transform -n "FPSR_readme_grp_see_notes_in_attribute_editor";
	rename -uid "5471C3BE-4CE1-8FCF-02F5-7095963B6628";
	addAttr -ci true -sn "nts" -ln "notes" -dt "string";
	setAttr ".nts" -type "string" (
		"// SPDX-License-Identifier: MIT — See LICENSE for full terms\n// Created by Patrick Woo, 2025.\n// This file is part of the FPS-R (Frame-Persistent Stateless Randomisation) project.\n// https://github.com/patwooky/FPSR_Algorithm\n\n\n// ============================================================================\n//\n//  FPS-R ALGORITHMS - SELF-CONTAINED MAYA EXPRESSIONS\n//\n// ============================================================================\n// FPS-R (Frame-Persistent Stateless Randomisation) is a set of algorithms that\n// generate frame-persistent and stateless random values. \n// This file contains two stateless, frame-persistent randomization algorithms.\n// It uses a custom portable_rand() function to ensure deterministic and\n// consistent results across any platform.\n//\n//  INSTRUCTIONS:\n//  1. Choose either the \"Stacked Modulo (SM)\" or \"Quantised Switching (QS)\"\n//     expression block below.\n//  2. Copy the ENTIRE block of code for your chosen algorithm.\n//  3. Paste it directly into the Maya Expression Editor for an attribute\n"
		+ "//     (e.g., pCube1.translateX).\n//  4. Modify the parameters and the final line (e.g., `pCube1.translateX = $randVal;`)\n//     to suit your needs.\n//\n//  These expressions are self-contained and do NOT require you to run\n//  anything in the Script Editor beforehand.\n//\n//  But they can also be converted to global procedures and run (once every session) \n//  and then called from unlimited expressions instances in that session, if you prefer.\n//\n// ============================================================================\n\n\n\n// ============================================================================\n// EXPRESSION 1: Stacked Modulo (SM) - Self-Contained\n// ============================================================================\n/******************************************************************************/\n/* FPS-R: Stacked Modulo (SM)                                                 */\n/******************************************************************************/\n/**\n * @brief Generates a persistent random value that holds for a calculated duration.\n"
		+ " * @details This function uses a two-step process. First, it determines a random\n * \"hold duration\". Second, it generates a stable integer for that duration,\n * which is then used as a seed to produce the final, held random value.\n *\n * int $frame: The current frame or time input. (In expressions, use the global 'frame' variable)\n * int $minHold: The minimum duration (in frames) for a value to hold.\n * int $maxHold: The maximum duration (in frames) for a value to hold.\n * int $reseedInterval: The fixed interval at which a new hold duration is calculated.\n * int $seedInner: An offset for the random duration calculation to create unique sequences.\n * int $seedOuter: An offset for the final value calculation to create unique sequences.\n * int $finalRandSwitch: A flag that can turn off the final randomisation step.\n * @return\n * when $finalRandSwitch is 0:\n * An integer value representing the currently held frame\n * that remains constant for the hold duration.\n * when $finalRandSwitch is 1:\n * A float value between 0.0 and 1.0 that remains constant for the hold duration.\n"
		+ " */\n// --- Start of Stacked Modulo (SM) Expression ---\n\n\n// A simple, portable pseudo-random number generator.\nproc float portable_rand(int $seed) {\n    // A common technique for a simple hash-like random number.\n    // The large prime numbers are used to create a chaotic, unpredictable result.\n    float $result = sin((float)$seed * 12.9898) * 43758.5453;\n    // Use `$result - floor($result)` as a replacement for frac() for older Maya versions.\n    // This ensures the result is always a positive value in the [0, 1) range.\n    return $result - floor($result);\n}\n\n\n// Generates a persistent random value that holds for a calculated duration.\nproc float fpsr_sm(\n    int $frame, int $minHold, int $maxHold,\n    int $reseedInterval, int $seedInner, int $seedOuter, int $finalRandSwitch)\n{\n    // --- 1. Calculate the random hold duration ---\n    if ($reseedInterval < 1) { $reseedInterval = 1; } // Prevent division by zero.\n\n\n    float $rand_for_duration = portable_rand($seedInner + $frame - ($frame % $reseedInterval));\n"
		+ "    int $holdDuration = (int)floor($minHold + $rand_for_duration * ($maxHold - $minHold));\n\n\n    if ($holdDuration < 1) { $holdDuration = 1; } // Prevent division by zero.\n\n\n    // --- 2. Generate the stable integer \"state\" for the hold period ---\n    int $held_integer_state = ($seedOuter + $frame) - (($seedOuter + $frame) % $holdDuration);\n\n\n    // --- 3. Use the stable state as a seed for the final random value ---\n    float $fpsr_output = 0.0;\n    if ($finalRandSwitch) {\n        $fpsr_output = portable_rand($held_integer_state);\n    } else {\n        $fpsr_output = $held_integer_state;\n    }\n    return $fpsr_output;\n}\n\n\n// --- Main Expression Logic (SM) ---\n\n\n// Parameters\nint $minHoldFrames = 16; // probable minimum held period\nint $maxHoldFrames = 24; // maximum held period before cycling\nint $reseedFrames = 9; // inner mod cycle timing\nint $offsetInner = -41; // offsets the inner frame\nint $offsetOuter = 23; // offsets the outer frame\nint $finalRandSwitch = 1; // 1 to apply the final randomisation step, 0 to skip it\n"
		+ "\n\n// Call the FPS-R:SM function\nfloat $randVal =\n    fpsr_sm(\n        frame, $minHoldFrames, $maxHoldFrames,\n        $reseedFrames, $offsetInner, $offsetOuter, $finalRandSwitch);\n\n\n/*\n// Optional: check if the value has changed from the previous frame\nfloat $randVal_previous =\n    fpsr_sm(\n        (frame-1), $minHoldFrames, $maxHoldFrames,\n        $reseedFrames, $offsetInner, $offsetOuter, $finalRandSwitch);\nint $changed = 0;\nif ($randVal != $randVal_previous) {\n    $changed = 1;\n}\n*/\n\n\n// ASSIGN THE FINAL VALUE to your object's attribute.\n// ** IMPORTANT: CHANGE \"pCube1.translateY\" to your target attribute! **\n// pCube1.translateY = $randVal;\n\n\n\n// --- End of Stacked Modulo (SM) Expression ---\n\n\n\n\n\n// ============================================================================\n// EXPRESSION 2: Quantised Switching (QS) - Self-Contained\n// ============================================================================\n/**\n * @brief Generates a flickering, quantised value by switching between two sine wave streams.\n"
		+ " * @details This function creates two separate, quantised sine waves and switches\n * between them at a fixed interval to create complex, glitch-like patterns.\n *\n * int $frame: The current frame or time input.\n * float $baseWaveFreq: The base frequency for the modulation wave of stream 1.\n * float $stream2FreqMult: A multiplier for the second stream's frequency. If < 0, a default is used.\n * int $quantLevelsMinMax[]: An array of two integers for the min and max quantisation levels.\n * int $streamsOffset[]: An array of two integers to offset the frame for each stream.\n * int $streamSwitchDur: The number of frames after which the streams switch. If < 1, a default is derived.\n * int $stream1QuantDur: The duration for stream 1's quantisation switch. If < 1, a default is derived.\n * int $stream2QuantDur: The duration for stream 2's quantisation switch. If < 1, a default is derived.\n * int $finalRandSwitch: A flag that can turn off the final randomisation step.\n * @return: A float value between 0.0 and 1.0 that remains constant for the hold duration.\n"
		+ " */\n// --- Start of Quantised Switching (QS) Expression ---\n\n\n// A simple, portable pseudo-random number generator.\nproc float portable_rand(int $seed) {\n    float $result = sin((float)$seed * 12.9898) * 43758.5453;\n    // Use `$result - floor($result)` as a replacement for frac() for older Maya versions.\n    // This ensures the result is always a positive value in the [0, 1) range.\n    return $result - floor($result);\n}\n\n\n// Generates a flickering, quantised value by switching between two sine wave streams.\nproc float fpsr_qs(\n    int $frame, float $baseWaveFreq, float $stream2FreqMult,\n    int $quantLevelsMinMax[], int $streamsOffset[],\n    int $streamSwitchDur, int $stream1QuantDur, int $stream2QuantDur,\n    int $finalRandSwitch)\n{\n    // --- 1. Set default durations if not provided ---\n    if ($streamSwitchDur < 1) { $streamSwitchDur = (int)floor((1.0 / $baseWaveFreq) * 0.76); }\n    if ($stream1QuantDur < 1) { $stream1QuantDur = (int)floor((1.0 / $baseWaveFreq) * 1.2); }\n    if ($stream2QuantDur < 1) { $stream2QuantDur = (int)floor((1.0 / $baseWaveFreq) * 0.9); }\n"
		+ "    if ($streamSwitchDur < 1) { $streamSwitchDur = 1; }\n    if ($stream1QuantDur < 1) { $stream1QuantDur = 1; }\n    if ($stream2QuantDur < 1) { $stream2QuantDur = 1; }\n\n\n    // --- 2. Calculate quantisation levels for each stream ---\n    int $s1_quant_level;\n    if (($streamsOffset[0] + $frame) % $stream1QuantDur < $stream1QuantDur * 0.5) {\n        $s1_quant_level = $quantLevelsMinMax[0];\n    } else {\n        $s1_quant_level = $quantLevelsMinMax[1];\n    }\n\n\n    int $s2_quant_level;\n    float $STREAM2_QUANT_RATIO_MIN = 1.24;\n    float $STREAM2_QUANT_RATIO_MAX = 0.66;\n    if (($streamsOffset[1] + $frame) % $stream2QuantDur < $stream2QuantDur * 0.5) {\n        $s2_quant_level = (int)floor($quantLevelsMinMax[0] * $STREAM2_QUANT_RATIO_MIN);\n    } else {\n        $s2_quant_level = (int)floor($quantLevelsMinMax[1] * $STREAM2_QUANT_RATIO_MAX);\n    }\n    if ($s1_quant_level < 1) { $s1_quant_level = 1; }\n    if ($s2_quant_level < 1) { $s2_quant_level = 1; }\n\n\n    // --- 3. Generate the two quantised sine wave streams ---\n"
		+ "    float $STREAM2_DEFAULT_FREQ_MULT = 3.7;\n    if ($stream2FreqMult < 0) { $stream2FreqMult = $STREAM2_DEFAULT_FREQ_MULT; }\n\n\n    float $stream1 = floor(sin((float)($streamsOffset[0] + $frame) * $baseWaveFreq) * $s1_quant_level) / $s1_quant_level;\n    float $stream2 = floor(sin((float)($streamsOffset[1] + $frame) * $baseWaveFreq * $stream2FreqMult) * $s2_quant_level) / $s2_quant_level;\n\n\n    // --- 4. Switch between the two streams ---\n    float $active_stream_val = 0.0;\n    if (($frame % $streamSwitchDur) < $streamSwitchDur / 2) {\n        $active_stream_val = $stream1;\n    } else {\n        $active_stream_val = $stream2;\n    }\n\n\n    // --- 5. Hash the final output to create a random-looking value ---\n    float $fpsr_output = 0.0;\n    if ($finalRandSwitch) {\n        $fpsr_output = portable_rand(int($active_stream_val * 100000.0));\n    } else {\n        $fpsr_output = 0.5 * $active_stream_val + 0.5; // Scale from [-1, 1] to [0, 1]\n    }\n    return $fpsr_output;\n}\n\n\n// --- Main Expression Logic (QS) ---\n\n\n// Parameters\n"
		+ "float $baseWaveFreq = 0.012; // Base frequency for the modulation wave of stream 1\nfloat $stream2freqMult = 3.1; // Multiplier for the second stream's frequency\nint $quantLevelsMinMax[] = {12, 22}; // Min, Max quantisation levels for the two streams\nint $streamsOffset[] = {0, 76}; // Offset for the two streams\nint $streamSwitchDur = 24; // Duration for switching streams in frames\nint $stream1QuantDur = 16; // Duration for the first stream's quantisation switch cycle in frames\nint $stream2QuantDur = 20; // Duration for the second stream's quantisation switch cycle in frames\nint $finalRandSwitch = 1; // 1 to apply the final randomisation step, 0 to skip it\n\n\n// Call the FPS-R:QS function\nfloat $randVal = fpsr_qs(\n    frame, $baseWaveFreq, $stream2freqMult, $quantLevelsMinMax,\n    $streamsOffset, $streamSwitchDur, $stream1QuantDur, $stream2QuantDur, $finalRandSwitch);\n\n\n/*\n// Optional: check if the value has changed from the previous frame\nfloat $randVal_previous = fpsr_qs(\n    (frame - 1), $baseWaveFreq, $stream2freqMult, $quantLevelsMinMax,\n"
		+ "    $streamsOffset, $streamSwitchDur, $stream1QuantDur, $stream2QuantDur, $finalRandSwitch);\nint $changed = 0;\nif ($randVal != $randVal_previous) {\n    $changed = 1;\n}\n*/\n\n\n// ASSIGN THE FINAL VALUE to your object's attribute.\n// ** IMPORTANT: CHANGE \"pCube1.visibility\" to your target attribute! **\n// pCube1.visibility = ($randVal > 0.5);\n\n\n\n// --- End of Quantised Switching (QS) Expression ---\n\n");
createNode transform -n "pCube_fpsr_sm" -p "FPSR_readme_grp_see_notes_in_attribute_editor";
	rename -uid "2C9016DE-445F-1F28-0E9F-D5AB68390FDA";
	addAttr -ci true -sn "changed" -ln "changed" -at "long";
	setAttr -k on ".changed";
createNode mesh -n "pCube_fpsr_smShape" -p "pCube_fpsr_sm";
	rename -uid "247D0FA2-47FC-502D-8F36-B8B704A8A525";
	setAttr -k off ".v";
	setAttr ".vir" yes;
	setAttr ".vif" yes;
	setAttr ".uvst[0].uvsn" -type "string" "map1";
	setAttr ".cuvs" -type "string" "map1";
	setAttr ".dcc" -type "string" "Ambient+Diffuse";
	setAttr ".covm[0]"  0 1 1;
	setAttr ".cdvm[0]"  0 1 1;
	setAttr ".ai_translator" -type "string" "polymesh";
createNode transform -n "pCube_fpsr_tm" -p "FPSR_readme_grp_see_notes_in_attribute_editor";
	rename -uid "48E4F097-4612-DC68-C799-609966AF8F3C";
	addAttr -ci true -sn "changed" -ln "changed" -at "long";
	setAttr ".t" -type "double3" 4 0 0 ;
	setAttr -k on ".changed";
createNode mesh -n "pCube_fpsr_tmShape" -p "pCube_fpsr_tm";
	rename -uid "DAEC7232-4F88-30AC-A521-F3BFC62E2F10";
	setAttr -k off ".v";
	setAttr ".vir" yes;
	setAttr ".vif" yes;
	setAttr ".uvst[0].uvsn" -type "string" "map1";
	setAttr ".cuvs" -type "string" "map1";
	setAttr ".dcc" -type "string" "Ambient+Diffuse";
	setAttr ".covm[0]"  0 1 1;
	setAttr ".cdvm[0]"  0 1 1;
	setAttr ".ai_translator" -type "string" "polymesh";
createNode transform -n "pCube_fpsr_qs" -p "FPSR_readme_grp_see_notes_in_attribute_editor";
	rename -uid "6A8A021F-4E53-5210-CE14-9FAD9AD4B1DD";
	addAttr -ci true -sn "changed" -ln "changed" -at "long";
	setAttr ".t" -type "double3" 8 0 0.8 ;
	setAttr -k on ".changed";
createNode mesh -n "pCube_fpsr_qsShape" -p "pCube_fpsr_qs";
	rename -uid "1E12973A-4828-DDB4-B1F8-6BA1A0FCAAAA";
	setAttr -k off ".v";
	setAttr ".vir" yes;
	setAttr ".vif" yes;
	setAttr ".uvst[0].uvsn" -type "string" "map1";
	setAttr -s 14 ".uvst[0].uvsp[0:13]" -type "float2" 0.375 0 0.625 0 0.375
		 0.25 0.625 0.25 0.375 0.5 0.625 0.5 0.375 0.75 0.625 0.75 0.375 1 0.625 1 0.875 0
		 0.875 0.25 0.125 0 0.125 0.25;
	setAttr ".cuvs" -type "string" "map1";
	setAttr ".dcc" -type "string" "Ambient+Diffuse";
	setAttr ".covm[0]"  0 1 1;
	setAttr ".cdvm[0]"  0 1 1;
	setAttr -s 8 ".vt[0:7]"  -0.5 -0.5 0.5 0.5 -0.5 0.5 -0.5 0.5 0.5 0.5 0.5 0.5
		 -0.5 0.5 -0.5 0.5 0.5 -0.5 -0.5 -0.5 -0.5 0.5 -0.5 -0.5;
	setAttr -s 12 ".ed[0:11]"  0 1 0 2 3 0 4 5 0 6 7 0 0 2 0 1 3 0 2 4 0
		 3 5 0 4 6 0 5 7 0 6 0 0 7 1 0;
	setAttr -s 6 -ch 24 ".fc[0:5]" -type "polyFaces" 
		f 4 0 5 -2 -5
		mu 0 4 0 1 3 2
		f 4 1 7 -3 -7
		mu 0 4 2 3 5 4
		f 4 2 9 -4 -9
		mu 0 4 4 5 7 6
		f 4 3 11 -1 -11
		mu 0 4 6 7 9 8
		f 4 -12 -10 -8 -6
		mu 0 4 1 10 11 3
		f 4 10 4 6 8
		mu 0 4 12 0 2 13;
	setAttr ".cd" -type "dataPolyComponent" Index_Data Edge 0 ;
	setAttr ".cvd" -type "dataPolyComponent" Index_Data Vertex 0 ;
	setAttr ".pd[0]" -type "dataPolyComponent" Index_Data UV 0 ;
	setAttr ".hfd" -type "dataPolyComponent" Index_Data Face 0 ;
	setAttr ".ai_translator" -type "string" "polymesh";
createNode transform -n "transform1";
	rename -uid "D7A97890-4360-94C7-EFA0-669B44B17EEC";
	setAttr ".hio" yes;
createNode displayPoints -n "displayPoints1" -p "transform1";
	rename -uid "9658678D-4ADF-13AE-1485-CC9385F2D9F7";
	setAttr -k off ".v";
	setAttr -s 2 ".inPointPositionsPP";
	setAttr ".hio" yes;
createNode transform -n "text_grp";
	rename -uid "2A25D125-43D9-863E-E156-8782DC9A9CED";
createNode transform -n "txt_fpsr_sm" -p "text_grp";
	rename -uid "7F113CD4-40BF-FE59-7B35-F0BA781253A2";
	setAttr ".t" -type "double3" 0 4.0171162855811264 0 ;
createNode mesh -n "txt_fpsr_smShape" -p "txt_fpsr_sm";
	rename -uid "C29B9AA8-4A5F-4526-64C3-D685B831D07B";
	setAttr -k off ".v";
	setAttr ".vir" yes;
	setAttr ".vif" yes;
	setAttr ".uvst[0].uvsn" -type "string" "map1";
	setAttr ".cuvs" -type "string" "map1";
	setAttr ".dcc" -type "string" "Ambient+Diffuse";
	setAttr ".covm[0]"  0 1 1;
	setAttr ".cdvm[0]"  0 1 1;
	setAttr ".ai_translator" -type "string" "polymesh";
createNode transform -n "txt_fpsr_tm" -p "text_grp";
	rename -uid "C9FB6EFD-4AC6-F5E3-E052-4B8D6E3ADC02";
	setAttr ".t" -type "double3" 4 4.0171162855811264 0 ;
createNode mesh -n "txt_fpsr_tmShape" -p "txt_fpsr_tm";
	rename -uid "414F4AFF-494D-7D09-0D84-BFAAC32F09AB";
	setAttr -k off ".v";
	setAttr ".vir" yes;
	setAttr ".vif" yes;
	setAttr ".uvst[0].uvsn" -type "string" "map1";
	setAttr ".cuvs" -type "string" "map1";
	setAttr ".dcc" -type "string" "Ambient+Diffuse";
	setAttr ".covm[0]"  0 1 1;
	setAttr ".cdvm[0]"  0 1 1;
	setAttr ".ai_translator" -type "string" "polymesh";
createNode transform -n "txt_fpsr_qs" -p "text_grp";
	rename -uid "554CDB67-432A-55E8-F2CF-4BBA09E88E66";
	setAttr ".t" -type "double3" 8 4.0171162855811264 0 ;
createNode mesh -n "txt_fpsr_qsShape" -p "txt_fpsr_qs";
	rename -uid "4D9742B5-4400-CACA-E025-90A6249B2FB8";
	setAttr -k off ".v";
	setAttr ".vir" yes;
	setAttr ".vif" yes;
	setAttr ".uvst[0].uvsn" -type "string" "map1";
	setAttr ".cuvs" -type "string" "map1";
	setAttr ".dcc" -type "string" "Ambient+Diffuse";
	setAttr ".covm[0]"  0 1 1;
	setAttr ".cdvm[0]"  0 1 1;
	setAttr ".ai_translator" -type "string" "polymesh";
createNode lightLinker -s -n "lightLinker1";
	rename -uid "1D124DB4-41B3-D39C-61AE-5CB81B752F98";
	setAttr -s 6 ".lnk";
	setAttr -s 6 ".slnk";
createNode shapeEditorManager -n "shapeEditorManager";
	rename -uid "B4FC89E9-4D74-18AF-E4D7-1AA01A47D3A7";
createNode poseInterpolatorManager -n "poseInterpolatorManager";
	rename -uid "A43AFD9E-4603-0AFA-F0DC-4B9C07B5F1CD";
createNode displayLayerManager -n "layerManager";
	rename -uid "69F59F63-4FF0-58A1-7595-8594A8ED61F1";
createNode displayLayer -n "defaultLayer";
	rename -uid "152BF485-409C-F682-CA99-DE88C916AB5D";
createNode renderLayerManager -n "renderLayerManager";
	rename -uid "547618DF-4DB0-9A22-BD07-AF92C9F1E63F";
createNode renderLayer -n "defaultRenderLayer";
	rename -uid "AF294C65-4CAD-5B7B-F2C6-AFA9A7BA5A3E";
	setAttr ".g" yes;
createNode polyCube -n "polyCube1";
	rename -uid "D42259BE-453E-A770-3649-0F828655D7DD";
	setAttr ".cuv" 4;
createNode script -n "uiConfigurationScriptNode";
	rename -uid "73389CD7-43B6-2FBA-02D5-56A4F388ADFB";
	setAttr ".b" -type "string" (
		"// Maya Mel UI Configuration File.\n//\n//  This script is machine generated.  Edit at your own risk.\n//\n//\n\nglobal string $gMainPane;\nif (`paneLayout -exists $gMainPane`) {\n\n\tglobal int $gUseScenePanelConfig;\n\tint    $useSceneConfig = $gUseScenePanelConfig;\n\tint    $nodeEditorPanelVisible = stringArrayContains(\"nodeEditorPanel1\", `getPanel -vis`);\n\tint    $nodeEditorWorkspaceControlOpen = (`workspaceControl -exists nodeEditorPanel1Window` && `workspaceControl -q -visible nodeEditorPanel1Window`);\n\tint    $menusOkayInPanels = `optionVar -q allowMenusInPanels`;\n\tint    $nVisPanes = `paneLayout -q -nvp $gMainPane`;\n\tint    $nPanes = 0;\n\tstring $editorName;\n\tstring $panelName;\n\tstring $itemFilterName;\n\tstring $panelConfig;\n\n\t//\n\t//  get current state of the UI\n\t//\n\tsceneUIReplacement -update $gMainPane;\n\n\t$panelName = `sceneUIReplacement -getNextPanel \"modelPanel\" (localizedPanelLabel(\"Top View\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tmodelPanel -edit -l (localizedPanelLabel(\"Top View\")) -mbv $menusOkayInPanels  $panelName;\n"
		+ "\t\t$editorName = $panelName;\n        modelEditor -e \n            -camera \"top\" \n            -useInteractiveMode 0\n            -displayLights \"default\" \n            -displayAppearance \"smoothShaded\" \n            -activeOnly 0\n            -ignorePanZoom 0\n            -wireframeOnShaded 0\n            -headsUpDisplay 1\n            -holdOuts 1\n            -selectionHiliteDisplay 1\n            -useDefaultMaterial 0\n            -bufferMode \"double\" \n            -twoSidedLighting 0\n            -backfaceCulling 0\n            -xray 0\n            -jointXray 0\n            -activeComponentsXray 0\n            -displayTextures 0\n            -smoothWireframe 0\n            -lineWidth 1\n            -textureAnisotropic 0\n            -textureHilight 1\n            -textureSampling 2\n            -textureDisplay \"modulate\" \n            -textureMaxSize 32768\n            -fogging 0\n            -fogSource \"fragment\" \n            -fogMode \"linear\" \n            -fogStart 0\n            -fogEnd 100\n            -fogDensity 0.1\n            -fogColor 0.5 0.5 0.5 1 \n"
		+ "            -depthOfFieldPreview 1\n            -maxConstantTransparency 1\n            -rendererName \"vp2Renderer\" \n            -objectFilterShowInHUD 1\n            -isFiltered 0\n            -colorResolution 256 256 \n            -bumpResolution 512 512 \n            -textureCompression 0\n            -transparencyAlgorithm \"frontAndBackCull\" \n            -transpInShadows 0\n            -cullingOverride \"none\" \n            -lowQualityLighting 0\n            -maximumNumHardwareLights 1\n            -occlusionCulling 0\n            -shadingModel 0\n            -useBaseRenderer 0\n            -useReducedRenderer 0\n            -smallObjectCulling 0\n            -smallObjectThreshold -1 \n            -interactiveDisableShadows 0\n            -interactiveBackFaceCull 0\n            -sortTransparent 1\n            -controllers 1\n            -nurbsCurves 1\n            -nurbsSurfaces 1\n            -polymeshes 1\n            -subdivSurfaces 1\n            -planes 1\n            -lights 1\n            -cameras 1\n            -controlVertices 1\n"
		+ "            -hulls 1\n            -grid 1\n            -imagePlane 1\n            -joints 1\n            -ikHandles 1\n            -deformers 1\n            -dynamics 1\n            -particleInstancers 1\n            -fluids 1\n            -hairSystems 1\n            -follicles 1\n            -nCloths 1\n            -nParticles 1\n            -nRigids 1\n            -dynamicConstraints 1\n            -locators 1\n            -manipulators 1\n            -pluginShapes 1\n            -dimensions 1\n            -handles 1\n            -pivots 1\n            -textures 1\n            -strokes 1\n            -motionTrails 1\n            -clipGhosts 1\n            -greasePencils 1\n            -shadows 0\n            -captureSequenceNumber -1\n            -width 1\n            -height 1\n            -sceneRenderFilter 0\n            $editorName;\n        modelEditor -e -viewSelected 0 $editorName;\n        modelEditor -e \n            -pluginObjects \"gpuCacheDisplayFilter\" 1 \n            $editorName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n"
		+ "\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextPanel \"modelPanel\" (localizedPanelLabel(\"Side View\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tmodelPanel -edit -l (localizedPanelLabel(\"Side View\")) -mbv $menusOkayInPanels  $panelName;\n\t\t$editorName = $panelName;\n        modelEditor -e \n            -camera \"side\" \n            -useInteractiveMode 0\n            -displayLights \"default\" \n            -displayAppearance \"smoothShaded\" \n            -activeOnly 0\n            -ignorePanZoom 0\n            -wireframeOnShaded 0\n            -headsUpDisplay 1\n            -holdOuts 1\n            -selectionHiliteDisplay 1\n            -useDefaultMaterial 0\n            -bufferMode \"double\" \n            -twoSidedLighting 0\n            -backfaceCulling 0\n            -xray 0\n            -jointXray 0\n            -activeComponentsXray 0\n            -displayTextures 0\n            -smoothWireframe 0\n            -lineWidth 1\n            -textureAnisotropic 0\n            -textureHilight 1\n            -textureSampling 2\n"
		+ "            -textureDisplay \"modulate\" \n            -textureMaxSize 32768\n            -fogging 0\n            -fogSource \"fragment\" \n            -fogMode \"linear\" \n            -fogStart 0\n            -fogEnd 100\n            -fogDensity 0.1\n            -fogColor 0.5 0.5 0.5 1 \n            -depthOfFieldPreview 1\n            -maxConstantTransparency 1\n            -rendererName \"vp2Renderer\" \n            -objectFilterShowInHUD 1\n            -isFiltered 0\n            -colorResolution 256 256 \n            -bumpResolution 512 512 \n            -textureCompression 0\n            -transparencyAlgorithm \"frontAndBackCull\" \n            -transpInShadows 0\n            -cullingOverride \"none\" \n            -lowQualityLighting 0\n            -maximumNumHardwareLights 1\n            -occlusionCulling 0\n            -shadingModel 0\n            -useBaseRenderer 0\n            -useReducedRenderer 0\n            -smallObjectCulling 0\n            -smallObjectThreshold -1 \n            -interactiveDisableShadows 0\n            -interactiveBackFaceCull 0\n"
		+ "            -sortTransparent 1\n            -controllers 1\n            -nurbsCurves 1\n            -nurbsSurfaces 1\n            -polymeshes 1\n            -subdivSurfaces 1\n            -planes 1\n            -lights 1\n            -cameras 1\n            -controlVertices 1\n            -hulls 1\n            -grid 1\n            -imagePlane 1\n            -joints 1\n            -ikHandles 1\n            -deformers 1\n            -dynamics 1\n            -particleInstancers 1\n            -fluids 1\n            -hairSystems 1\n            -follicles 1\n            -nCloths 1\n            -nParticles 1\n            -nRigids 1\n            -dynamicConstraints 1\n            -locators 1\n            -manipulators 1\n            -pluginShapes 1\n            -dimensions 1\n            -handles 1\n            -pivots 1\n            -textures 1\n            -strokes 1\n            -motionTrails 1\n            -clipGhosts 1\n            -greasePencils 1\n            -shadows 0\n            -captureSequenceNumber -1\n            -width 1\n            -height 1\n"
		+ "            -sceneRenderFilter 0\n            $editorName;\n        modelEditor -e -viewSelected 0 $editorName;\n        modelEditor -e \n            -pluginObjects \"gpuCacheDisplayFilter\" 1 \n            $editorName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextPanel \"modelPanel\" (localizedPanelLabel(\"Front View\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tmodelPanel -edit -l (localizedPanelLabel(\"Front View\")) -mbv $menusOkayInPanels  $panelName;\n\t\t$editorName = $panelName;\n        modelEditor -e \n            -camera \"front\" \n            -useInteractiveMode 0\n            -displayLights \"default\" \n            -displayAppearance \"smoothShaded\" \n            -activeOnly 0\n            -ignorePanZoom 0\n            -wireframeOnShaded 0\n            -headsUpDisplay 1\n            -holdOuts 1\n            -selectionHiliteDisplay 1\n            -useDefaultMaterial 0\n            -bufferMode \"double\" \n            -twoSidedLighting 0\n            -backfaceCulling 0\n"
		+ "            -xray 0\n            -jointXray 0\n            -activeComponentsXray 0\n            -displayTextures 0\n            -smoothWireframe 0\n            -lineWidth 1\n            -textureAnisotropic 0\n            -textureHilight 1\n            -textureSampling 2\n            -textureDisplay \"modulate\" \n            -textureMaxSize 32768\n            -fogging 0\n            -fogSource \"fragment\" \n            -fogMode \"linear\" \n            -fogStart 0\n            -fogEnd 100\n            -fogDensity 0.1\n            -fogColor 0.5 0.5 0.5 1 \n            -depthOfFieldPreview 1\n            -maxConstantTransparency 1\n            -rendererName \"vp2Renderer\" \n            -objectFilterShowInHUD 1\n            -isFiltered 0\n            -colorResolution 256 256 \n            -bumpResolution 512 512 \n            -textureCompression 0\n            -transparencyAlgorithm \"frontAndBackCull\" \n            -transpInShadows 0\n            -cullingOverride \"none\" \n            -lowQualityLighting 0\n            -maximumNumHardwareLights 1\n            -occlusionCulling 0\n"
		+ "            -shadingModel 0\n            -useBaseRenderer 0\n            -useReducedRenderer 0\n            -smallObjectCulling 0\n            -smallObjectThreshold -1 \n            -interactiveDisableShadows 0\n            -interactiveBackFaceCull 0\n            -sortTransparent 1\n            -controllers 1\n            -nurbsCurves 1\n            -nurbsSurfaces 1\n            -polymeshes 1\n            -subdivSurfaces 1\n            -planes 1\n            -lights 1\n            -cameras 1\n            -controlVertices 1\n            -hulls 1\n            -grid 1\n            -imagePlane 1\n            -joints 1\n            -ikHandles 1\n            -deformers 1\n            -dynamics 1\n            -particleInstancers 1\n            -fluids 1\n            -hairSystems 1\n            -follicles 1\n            -nCloths 1\n            -nParticles 1\n            -nRigids 1\n            -dynamicConstraints 1\n            -locators 1\n            -manipulators 1\n            -pluginShapes 1\n            -dimensions 1\n            -handles 1\n            -pivots 1\n"
		+ "            -textures 1\n            -strokes 1\n            -motionTrails 1\n            -clipGhosts 1\n            -greasePencils 1\n            -shadows 0\n            -captureSequenceNumber -1\n            -width 1\n            -height 1\n            -sceneRenderFilter 0\n            $editorName;\n        modelEditor -e -viewSelected 0 $editorName;\n        modelEditor -e \n            -pluginObjects \"gpuCacheDisplayFilter\" 1 \n            $editorName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextPanel \"modelPanel\" (localizedPanelLabel(\"Persp View\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tmodelPanel -edit -l (localizedPanelLabel(\"Persp View\")) -mbv $menusOkayInPanels  $panelName;\n\t\t$editorName = $panelName;\n        modelEditor -e \n            -camera \"persp\" \n            -useInteractiveMode 0\n            -displayLights \"default\" \n            -displayAppearance \"smoothShaded\" \n            -activeOnly 0\n            -ignorePanZoom 0\n"
		+ "            -wireframeOnShaded 0\n            -headsUpDisplay 1\n            -holdOuts 1\n            -selectionHiliteDisplay 1\n            -useDefaultMaterial 0\n            -bufferMode \"double\" \n            -twoSidedLighting 0\n            -backfaceCulling 0\n            -xray 0\n            -jointXray 0\n            -activeComponentsXray 0\n            -displayTextures 1\n            -smoothWireframe 0\n            -lineWidth 1\n            -textureAnisotropic 0\n            -textureHilight 1\n            -textureSampling 2\n            -textureDisplay \"modulate\" \n            -textureMaxSize 32768\n            -fogging 0\n            -fogSource \"fragment\" \n            -fogMode \"linear\" \n            -fogStart 0\n            -fogEnd 100\n            -fogDensity 0.1\n            -fogColor 0.5 0.5 0.5 1 \n            -depthOfFieldPreview 1\n            -maxConstantTransparency 1\n            -rendererName \"vp2Renderer\" \n            -objectFilterShowInHUD 1\n            -isFiltered 0\n            -colorResolution 256 256 \n            -bumpResolution 512 512 \n"
		+ "            -textureCompression 0\n            -transparencyAlgorithm \"frontAndBackCull\" \n            -transpInShadows 0\n            -cullingOverride \"none\" \n            -lowQualityLighting 0\n            -maximumNumHardwareLights 1\n            -occlusionCulling 0\n            -shadingModel 0\n            -useBaseRenderer 0\n            -useReducedRenderer 0\n            -smallObjectCulling 0\n            -smallObjectThreshold -1 \n            -interactiveDisableShadows 0\n            -interactiveBackFaceCull 0\n            -sortTransparent 1\n            -controllers 1\n            -nurbsCurves 1\n            -nurbsSurfaces 1\n            -polymeshes 1\n            -subdivSurfaces 1\n            -planes 1\n            -lights 1\n            -cameras 1\n            -controlVertices 1\n            -hulls 1\n            -grid 1\n            -imagePlane 1\n            -joints 1\n            -ikHandles 1\n            -deformers 1\n            -dynamics 1\n            -particleInstancers 1\n            -fluids 1\n            -hairSystems 1\n            -follicles 1\n"
		+ "            -nCloths 1\n            -nParticles 1\n            -nRigids 1\n            -dynamicConstraints 1\n            -locators 1\n            -manipulators 1\n            -pluginShapes 1\n            -dimensions 1\n            -handles 1\n            -pivots 1\n            -textures 1\n            -strokes 1\n            -motionTrails 1\n            -clipGhosts 1\n            -greasePencils 1\n            -shadows 0\n            -captureSequenceNumber -1\n            -width 1173\n            -height 686\n            -sceneRenderFilter 0\n            $editorName;\n        modelEditor -e -viewSelected 0 $editorName;\n        modelEditor -e \n            -pluginObjects \"gpuCacheDisplayFilter\" 1 \n            $editorName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextPanel \"outlinerPanel\" (localizedPanelLabel(\"ToggledOutliner\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\toutlinerPanel -edit -l (localizedPanelLabel(\"ToggledOutliner\")) -mbv $menusOkayInPanels  $panelName;\n"
		+ "\t\t$editorName = $panelName;\n        outlinerEditor -e \n            -docTag \"isolOutln_fromSeln\" \n            -showShapes 0\n            -showAssignedMaterials 0\n            -showTimeEditor 1\n            -showReferenceNodes 1\n            -showReferenceMembers 1\n            -showAttributes 0\n            -showConnected 0\n            -showAnimCurvesOnly 0\n            -showMuteInfo 0\n            -organizeByLayer 1\n            -organizeByClip 1\n            -showAnimLayerWeight 1\n            -autoExpandLayers 1\n            -autoExpand 0\n            -showDagOnly 1\n            -showAssets 1\n            -showContainedOnly 1\n            -showPublishedAsConnected 0\n            -showParentContainers 0\n            -showContainerContents 1\n            -ignoreDagHierarchy 0\n            -expandConnections 0\n            -showUpstreamCurves 1\n            -showUnitlessCurves 1\n            -showCompounds 1\n            -showLeafs 1\n            -showNumericAttrsOnly 0\n            -highlightActive 1\n            -autoSelectNewObjects 0\n"
		+ "            -doNotSelectNewObjects 0\n            -dropIsParent 1\n            -transmitFilters 0\n            -setFilter \"defaultSetFilter\" \n            -showSetMembers 1\n            -allowMultiSelection 1\n            -alwaysToggleSelect 0\n            -directSelect 0\n            -isSet 0\n            -isSetMember 0\n            -displayMode \"DAG\" \n            -expandObjects 0\n            -setsIgnoreFilters 1\n            -containersIgnoreFilters 0\n            -editAttrName 0\n            -showAttrValues 0\n            -highlightSecondary 0\n            -showUVAttrsOnly 0\n            -showTextureNodesOnly 0\n            -attrAlphaOrder \"default\" \n            -animLayerFilterOptions \"allAffecting\" \n            -sortOrder \"none\" \n            -longNames 0\n            -niceNames 1\n            -selectCommand \"print(\\\"\\\")\" \n            -showNamespace 1\n            -showPinIcons 0\n            -mapMotionTrails 0\n            -ignoreHiddenAttribute 0\n            -ignoreOutlinerColor 0\n            -renderFilterVisible 0\n            -renderFilterIndex 0\n"
		+ "            -selectionOrder \"chronological\" \n            -expandAttribute 0\n            $editorName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextPanel \"outlinerPanel\" (localizedPanelLabel(\"Outliner\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\toutlinerPanel -edit -l (localizedPanelLabel(\"Outliner\")) -mbv $menusOkayInPanels  $panelName;\n\t\t$editorName = $panelName;\n        outlinerEditor -e \n            -showShapes 0\n            -showAssignedMaterials 0\n            -showTimeEditor 1\n            -showReferenceNodes 0\n            -showReferenceMembers 0\n            -showAttributes 0\n            -showConnected 0\n            -showAnimCurvesOnly 0\n            -showMuteInfo 0\n            -organizeByLayer 1\n            -organizeByClip 1\n            -showAnimLayerWeight 1\n            -autoExpandLayers 1\n            -autoExpand 0\n            -showDagOnly 1\n            -showAssets 1\n            -showContainedOnly 1\n            -showPublishedAsConnected 0\n"
		+ "            -showParentContainers 0\n            -showContainerContents 1\n            -ignoreDagHierarchy 0\n            -expandConnections 0\n            -showUpstreamCurves 1\n            -showUnitlessCurves 1\n            -showCompounds 1\n            -showLeafs 1\n            -showNumericAttrsOnly 0\n            -highlightActive 1\n            -autoSelectNewObjects 0\n            -doNotSelectNewObjects 0\n            -dropIsParent 1\n            -transmitFilters 0\n            -setFilter \"defaultSetFilter\" \n            -showSetMembers 1\n            -allowMultiSelection 1\n            -alwaysToggleSelect 0\n            -directSelect 0\n            -displayMode \"DAG\" \n            -expandObjects 0\n            -setsIgnoreFilters 1\n            -containersIgnoreFilters 0\n            -editAttrName 0\n            -showAttrValues 0\n            -highlightSecondary 0\n            -showUVAttrsOnly 0\n            -showTextureNodesOnly 0\n            -attrAlphaOrder \"default\" \n            -animLayerFilterOptions \"allAffecting\" \n            -sortOrder \"none\" \n"
		+ "            -longNames 0\n            -niceNames 1\n            -showNamespace 1\n            -showPinIcons 0\n            -mapMotionTrails 0\n            -ignoreHiddenAttribute 0\n            -ignoreOutlinerColor 0\n            -renderFilterVisible 0\n            $editorName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"graphEditor\" (localizedPanelLabel(\"Graph Editor\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Graph Editor\")) -mbv $menusOkayInPanels  $panelName;\n\n\t\t\t$editorName = ($panelName+\"OutlineEd\");\n            outlinerEditor -e \n                -showShapes 1\n                -showAssignedMaterials 0\n                -showTimeEditor 1\n                -showReferenceNodes 0\n                -showReferenceMembers 0\n                -showAttributes 1\n                -showConnected 1\n                -showAnimCurvesOnly 1\n                -showMuteInfo 0\n                -organizeByLayer 1\n"
		+ "                -organizeByClip 1\n                -showAnimLayerWeight 1\n                -autoExpandLayers 1\n                -autoExpand 1\n                -showDagOnly 0\n                -showAssets 1\n                -showContainedOnly 0\n                -showPublishedAsConnected 0\n                -showParentContainers 0\n                -showContainerContents 0\n                -ignoreDagHierarchy 0\n                -expandConnections 1\n                -showUpstreamCurves 1\n                -showUnitlessCurves 1\n                -showCompounds 0\n                -showLeafs 1\n                -showNumericAttrsOnly 1\n                -highlightActive 0\n                -autoSelectNewObjects 1\n                -doNotSelectNewObjects 0\n                -dropIsParent 1\n                -transmitFilters 1\n                -setFilter \"0\" \n                -showSetMembers 0\n                -allowMultiSelection 1\n                -alwaysToggleSelect 0\n                -directSelect 0\n                -isSet 0\n                -isSetMember 0\n"
		+ "                -displayMode \"DAG\" \n                -expandObjects 0\n                -setsIgnoreFilters 1\n                -containersIgnoreFilters 0\n                -editAttrName 0\n                -showAttrValues 0\n                -highlightSecondary 0\n                -showUVAttrsOnly 0\n                -showTextureNodesOnly 0\n                -attrAlphaOrder \"default\" \n                -animLayerFilterOptions \"allAffecting\" \n                -sortOrder \"none\" \n                -longNames 0\n                -niceNames 1\n                -showNamespace 1\n                -showPinIcons 1\n                -mapMotionTrails 1\n                -ignoreHiddenAttribute 0\n                -ignoreOutlinerColor 0\n                -renderFilterVisible 0\n                -selectionOrder \"display\" \n                -expandAttribute 1\n                $editorName;\n\n\t\t\t$editorName = ($panelName+\"GraphEd\");\n            animCurveEditor -e \n                -displayValues 0\n                -snapTime \"integer\" \n                -snapValue \"none\" \n"
		+ "                -showPlayRangeShades \"on\" \n                -lockPlayRangeShades \"off\" \n                -smoothness \"fine\" \n                -resultSamples 1\n                -resultScreenSamples 0\n                -resultUpdate \"delayed\" \n                -showUpstreamCurves 1\n                -keyMinScale 1\n                -stackedCurvesMin -1\n                -stackedCurvesMax 1\n                -stackedCurvesSpace 0.2\n                -preSelectionHighlight 0\n                -constrainDrag 0\n                -valueLinesToggle 0\n                -outliner \"graphEditor1OutlineEd\" \n                -highlightAffectedCurves 0\n                $editorName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"dopeSheetPanel\" (localizedPanelLabel(\"Dope Sheet\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Dope Sheet\")) -mbv $menusOkayInPanels  $panelName;\n\n\t\t\t$editorName = ($panelName+\"OutlineEd\");\n"
		+ "            outlinerEditor -e \n                -showShapes 1\n                -showAssignedMaterials 0\n                -showTimeEditor 1\n                -showReferenceNodes 0\n                -showReferenceMembers 0\n                -showAttributes 1\n                -showConnected 1\n                -showAnimCurvesOnly 1\n                -showMuteInfo 0\n                -organizeByLayer 1\n                -organizeByClip 1\n                -showAnimLayerWeight 1\n                -autoExpandLayers 1\n                -autoExpand 0\n                -showDagOnly 0\n                -showAssets 1\n                -showContainedOnly 0\n                -showPublishedAsConnected 0\n                -showParentContainers 0\n                -showContainerContents 0\n                -ignoreDagHierarchy 0\n                -expandConnections 1\n                -showUpstreamCurves 1\n                -showUnitlessCurves 0\n                -showCompounds 1\n                -showLeafs 1\n                -showNumericAttrsOnly 1\n                -highlightActive 0\n"
		+ "                -autoSelectNewObjects 0\n                -doNotSelectNewObjects 1\n                -dropIsParent 1\n                -transmitFilters 0\n                -setFilter \"0\" \n                -showSetMembers 0\n                -allowMultiSelection 1\n                -alwaysToggleSelect 0\n                -directSelect 0\n                -displayMode \"DAG\" \n                -expandObjects 0\n                -setsIgnoreFilters 1\n                -containersIgnoreFilters 0\n                -editAttrName 0\n                -showAttrValues 0\n                -highlightSecondary 0\n                -showUVAttrsOnly 0\n                -showTextureNodesOnly 0\n                -attrAlphaOrder \"default\" \n                -animLayerFilterOptions \"allAffecting\" \n                -sortOrder \"none\" \n                -longNames 0\n                -niceNames 1\n                -showNamespace 1\n                -showPinIcons 0\n                -mapMotionTrails 1\n                -ignoreHiddenAttribute 0\n                -ignoreOutlinerColor 0\n                -renderFilterVisible 0\n"
		+ "                $editorName;\n\n\t\t\t$editorName = ($panelName+\"DopeSheetEd\");\n            dopeSheetEditor -e \n                -displayValues 0\n                -snapTime \"integer\" \n                -snapValue \"none\" \n                -outliner \"dopeSheetPanel1OutlineEd\" \n                -showSummary 1\n                -showScene 0\n                -hierarchyBelow 0\n                -showTicks 1\n                -selectionWindow 0 0 0 0 \n                $editorName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"timeEditorPanel\" (localizedPanelLabel(\"Time Editor\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Time Editor\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"clipEditorPanel\" (localizedPanelLabel(\"Trax Editor\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n"
		+ "\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Trax Editor\")) -mbv $menusOkayInPanels  $panelName;\n\n\t\t\t$editorName = clipEditorNameFromPanel($panelName);\n            clipEditor -e \n                -displayValues 0\n                -snapTime \"none\" \n                -snapValue \"none\" \n                -initialized 0\n                -manageSequencer 0 \n                $editorName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"sequenceEditorPanel\" (localizedPanelLabel(\"Camera Sequencer\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Camera Sequencer\")) -mbv $menusOkayInPanels  $panelName;\n\n\t\t\t$editorName = sequenceEditorNameFromPanel($panelName);\n            clipEditor -e \n                -displayValues 0\n                -snapTime \"none\" \n                -snapValue \"none\" \n                -initialized 0\n                -manageSequencer 1 \n                $editorName;\n"
		+ "\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"hyperGraphPanel\" (localizedPanelLabel(\"Hypergraph Hierarchy\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Hypergraph Hierarchy\")) -mbv $menusOkayInPanels  $panelName;\n\n\t\t\t$editorName = ($panelName+\"HyperGraphEd\");\n            hyperGraph -e \n                -graphLayoutStyle \"hierarchicalLayout\" \n                -orientation \"horiz\" \n                -mergeConnections 0\n                -zoom 1\n                -animateTransition 0\n                -showRelationships 1\n                -showShapes 0\n                -showDeformers 0\n                -showExpressions 0\n                -showConstraints 0\n                -showConnectionFromSelected 0\n                -showConnectionToSelected 0\n                -showConstraintLabels 0\n                -showUnderworld 0\n                -showInvisible 0\n                -transitionFrames 1\n"
		+ "                -opaqueContainers 0\n                -freeform 0\n                -imagePosition 0 0 \n                -imageScale 1\n                -imageEnabled 0\n                -graphType \"DAG\" \n                -heatMapDisplay 0\n                -updateSelection 1\n                -updateNodeAdded 1\n                -useDrawOverrideColor 0\n                -limitGraphTraversal -1\n                -range 0 0 \n                -iconSize \"smallIcons\" \n                -showCachedConnections 0\n                $editorName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"hyperShadePanel\" (localizedPanelLabel(\"Hypershade\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Hypershade\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"visorPanel\" (localizedPanelLabel(\"Visor\")) `;\n"
		+ "\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Visor\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"nodeEditorPanel\" (localizedPanelLabel(\"Node Editor\")) `;\n\tif ($nodeEditorPanelVisible || $nodeEditorWorkspaceControlOpen) {\n\t\tif (\"\" == $panelName) {\n\t\t\tif ($useSceneConfig) {\n\t\t\t\t$panelName = `scriptedPanel -unParent  -type \"nodeEditorPanel\" -l (localizedPanelLabel(\"Node Editor\")) -mbv $menusOkayInPanels `;\n\n\t\t\t$editorName = ($panelName+\"NodeEditorEd\");\n            nodeEditor -e \n                -allAttributes 0\n                -allNodes 0\n                -autoSizeNodes 1\n                -consistentNameSize 1\n                -createNodeCommand \"nodeEdCreateNodeCommand\" \n                -connectNodeOnCreation 0\n                -connectOnDrop 0\n                -copyConnectionsOnPaste 0\n                -connectionStyle \"bezier\" \n                -defaultPinnedState 0\n"
		+ "                -additiveGraphingMode 0\n                -settingsChangedCallback \"nodeEdSyncControls\" \n                -traversalDepthLimit -1\n                -keyPressCommand \"nodeEdKeyPressCommand\" \n                -nodeTitleMode \"name\" \n                -gridSnap 0\n                -gridVisibility 1\n                -crosshairOnEdgeDragging 0\n                -popupMenuScript \"nodeEdBuildPanelMenus\" \n                -showNamespace 1\n                -showShapes 1\n                -showSGShapes 0\n                -showTransforms 1\n                -useAssets 1\n                -syncedSelection 1\n                -extendToShapes 1\n                -editorMode \"default\" \n                -hasWatchpoint 0\n                $editorName;\n\t\t\t}\n\t\t} else {\n\t\t\t$label = `panel -q -label $panelName`;\n\t\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Node Editor\")) -mbv $menusOkayInPanels  $panelName;\n\n\t\t\t$editorName = ($panelName+\"NodeEditorEd\");\n            nodeEditor -e \n                -allAttributes 0\n                -allNodes 0\n                -autoSizeNodes 1\n"
		+ "                -consistentNameSize 1\n                -createNodeCommand \"nodeEdCreateNodeCommand\" \n                -connectNodeOnCreation 0\n                -connectOnDrop 0\n                -copyConnectionsOnPaste 0\n                -connectionStyle \"bezier\" \n                -defaultPinnedState 0\n                -additiveGraphingMode 0\n                -settingsChangedCallback \"nodeEdSyncControls\" \n                -traversalDepthLimit -1\n                -keyPressCommand \"nodeEdKeyPressCommand\" \n                -nodeTitleMode \"name\" \n                -gridSnap 0\n                -gridVisibility 1\n                -crosshairOnEdgeDragging 0\n                -popupMenuScript \"nodeEdBuildPanelMenus\" \n                -showNamespace 1\n                -showShapes 1\n                -showSGShapes 0\n                -showTransforms 1\n                -useAssets 1\n                -syncedSelection 1\n                -extendToShapes 1\n                -editorMode \"default\" \n                -hasWatchpoint 0\n                $editorName;\n"
		+ "\t\t\tif (!$useSceneConfig) {\n\t\t\t\tpanel -e -l $label $panelName;\n\t\t\t}\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"createNodePanel\" (localizedPanelLabel(\"Create Node\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Create Node\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"polyTexturePlacementPanel\" (localizedPanelLabel(\"UV Editor\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"UV Editor\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"renderWindowPanel\" (localizedPanelLabel(\"Render View\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Render View\")) -mbv $menusOkayInPanels  $panelName;\n"
		+ "\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextPanel \"shapePanel\" (localizedPanelLabel(\"Shape Editor\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tshapePanel -edit -l (localizedPanelLabel(\"Shape Editor\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextPanel \"posePanel\" (localizedPanelLabel(\"Pose Editor\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tposePanel -edit -l (localizedPanelLabel(\"Pose Editor\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"dynRelEdPanel\" (localizedPanelLabel(\"Dynamic Relationships\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Dynamic Relationships\")) -mbv $menusOkayInPanels  $panelName;\n"
		+ "\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"relationshipPanel\" (localizedPanelLabel(\"Relationship Editor\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Relationship Editor\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"referenceEditorPanel\" (localizedPanelLabel(\"Reference Editor\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Reference Editor\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"componentEditorPanel\" (localizedPanelLabel(\"Component Editor\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Component Editor\")) -mbv $menusOkayInPanels  $panelName;\n"
		+ "\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"dynPaintScriptedPanelType\" (localizedPanelLabel(\"Paint Effects\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Paint Effects\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"scriptEditorPanel\" (localizedPanelLabel(\"Script Editor\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Script Editor\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"profilerPanel\" (localizedPanelLabel(\"Profiler Tool\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Profiler Tool\")) -mbv $menusOkayInPanels  $panelName;\n"
		+ "\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"contentBrowserPanel\" (localizedPanelLabel(\"Content Browser\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Content Browser\")) -mbv $menusOkayInPanels  $panelName;\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\t$panelName = `sceneUIReplacement -getNextScriptedPanel \"Stereo\" (localizedPanelLabel(\"Stereo\")) `;\n\tif (\"\" != $panelName) {\n\t\t$label = `panel -q -label $panelName`;\n\t\tscriptedPanel -edit -l (localizedPanelLabel(\"Stereo\")) -mbv $menusOkayInPanels  $panelName;\n{ string $editorName = ($panelName+\"Editor\");\n            stereoCameraView -e \n                -camera \"persp\" \n                -useInteractiveMode 0\n                -displayLights \"default\" \n                -displayAppearance \"wireframe\" \n                -activeOnly 0\n                -ignorePanZoom 0\n                -wireframeOnShaded 0\n"
		+ "                -headsUpDisplay 1\n                -holdOuts 1\n                -selectionHiliteDisplay 1\n                -useDefaultMaterial 0\n                -bufferMode \"double\" \n                -twoSidedLighting 1\n                -backfaceCulling 0\n                -xray 0\n                -jointXray 0\n                -activeComponentsXray 0\n                -displayTextures 0\n                -smoothWireframe 0\n                -lineWidth 1\n                -textureAnisotropic 0\n                -textureHilight 1\n                -textureSampling 2\n                -textureDisplay \"modulate\" \n                -textureMaxSize 32768\n                -fogging 0\n                -fogSource \"fragment\" \n                -fogMode \"linear\" \n                -fogStart 0\n                -fogEnd 100\n                -fogDensity 0.1\n                -fogColor 0.5 0.5 0.5 1 \n                -depthOfFieldPreview 1\n                -maxConstantTransparency 1\n                -objectFilterShowInHUD 1\n                -isFiltered 0\n                -colorResolution 4 4 \n"
		+ "                -bumpResolution 4 4 \n                -textureCompression 0\n                -transparencyAlgorithm \"frontAndBackCull\" \n                -transpInShadows 0\n                -cullingOverride \"none\" \n                -lowQualityLighting 0\n                -maximumNumHardwareLights 0\n                -occlusionCulling 0\n                -shadingModel 0\n                -useBaseRenderer 0\n                -useReducedRenderer 0\n                -smallObjectCulling 0\n                -smallObjectThreshold -1 \n                -interactiveDisableShadows 0\n                -interactiveBackFaceCull 0\n                -sortTransparent 1\n                -controllers 1\n                -nurbsCurves 1\n                -nurbsSurfaces 1\n                -polymeshes 1\n                -subdivSurfaces 1\n                -planes 1\n                -lights 1\n                -cameras 1\n                -controlVertices 1\n                -hulls 1\n                -grid 1\n                -imagePlane 1\n                -joints 1\n                -ikHandles 1\n"
		+ "                -deformers 1\n                -dynamics 1\n                -particleInstancers 1\n                -fluids 1\n                -hairSystems 1\n                -follicles 1\n                -nCloths 1\n                -nParticles 1\n                -nRigids 1\n                -dynamicConstraints 1\n                -locators 1\n                -manipulators 1\n                -pluginShapes 1\n                -dimensions 1\n                -handles 1\n                -pivots 1\n                -textures 1\n                -strokes 1\n                -motionTrails 1\n                -clipGhosts 1\n                -greasePencils 1\n                -shadows 0\n                -captureSequenceNumber -1\n                -width 0\n                -height 0\n                -sceneRenderFilter 0\n                -displayMode \"centerEye\" \n                -viewColor 0 0 0 1 \n                -useCustomBackground 1\n                $editorName;\n            stereoCameraView -e -viewSelected 0 $editorName;\n            stereoCameraView -e \n"
		+ "                -pluginObjects \"gpuCacheDisplayFilter\" 1 \n                $editorName; };\n\t\tif (!$useSceneConfig) {\n\t\t\tpanel -e -l $label $panelName;\n\t\t}\n\t}\n\n\n\tif ($useSceneConfig) {\n        string $configName = `getPanel -cwl (localizedPanelLabel(\"Current Layout\"))`;\n        if (\"\" != $configName) {\n\t\t\tpanelConfiguration -edit -label (localizedPanelLabel(\"Current Layout\")) \n\t\t\t\t-userCreated false\n\t\t\t\t-defaultImage \"vacantCell.xP:/\"\n\t\t\t\t-image \"\"\n\t\t\t\t-sc false\n\t\t\t\t-configString \"global string $gMainPane; paneLayout -e -cn \\\"single\\\" -ps 1 100 100 $gMainPane;\"\n\t\t\t\t-removeAllPanels\n\t\t\t\t-ap false\n\t\t\t\t\t(localizedPanelLabel(\"Persp View\")) \n\t\t\t\t\t\"modelPanel\"\n"
		+ "\t\t\t\t\t\"$panelName = `modelPanel -unParent -l (localizedPanelLabel(\\\"Persp View\\\")) -mbv $menusOkayInPanels `;\\n$editorName = $panelName;\\nmodelEditor -e \\n    -cam `findStartUpCamera persp` \\n    -useInteractiveMode 0\\n    -displayLights \\\"default\\\" \\n    -displayAppearance \\\"smoothShaded\\\" \\n    -activeOnly 0\\n    -ignorePanZoom 0\\n    -wireframeOnShaded 0\\n    -headsUpDisplay 1\\n    -holdOuts 1\\n    -selectionHiliteDisplay 1\\n    -useDefaultMaterial 0\\n    -bufferMode \\\"double\\\" \\n    -twoSidedLighting 0\\n    -backfaceCulling 0\\n    -xray 0\\n    -jointXray 0\\n    -activeComponentsXray 0\\n    -displayTextures 1\\n    -smoothWireframe 0\\n    -lineWidth 1\\n    -textureAnisotropic 0\\n    -textureHilight 1\\n    -textureSampling 2\\n    -textureDisplay \\\"modulate\\\" \\n    -textureMaxSize 32768\\n    -fogging 0\\n    -fogSource \\\"fragment\\\" \\n    -fogMode \\\"linear\\\" \\n    -fogStart 0\\n    -fogEnd 100\\n    -fogDensity 0.1\\n    -fogColor 0.5 0.5 0.5 1 \\n    -depthOfFieldPreview 1\\n    -maxConstantTransparency 1\\n    -rendererName \\\"vp2Renderer\\\" \\n    -objectFilterShowInHUD 1\\n    -isFiltered 0\\n    -colorResolution 256 256 \\n    -bumpResolution 512 512 \\n    -textureCompression 0\\n    -transparencyAlgorithm \\\"frontAndBackCull\\\" \\n    -transpInShadows 0\\n    -cullingOverride \\\"none\\\" \\n    -lowQualityLighting 0\\n    -maximumNumHardwareLights 1\\n    -occlusionCulling 0\\n    -shadingModel 0\\n    -useBaseRenderer 0\\n    -useReducedRenderer 0\\n    -smallObjectCulling 0\\n    -smallObjectThreshold -1 \\n    -interactiveDisableShadows 0\\n    -interactiveBackFaceCull 0\\n    -sortTransparent 1\\n    -controllers 1\\n    -nurbsCurves 1\\n    -nurbsSurfaces 1\\n    -polymeshes 1\\n    -subdivSurfaces 1\\n    -planes 1\\n    -lights 1\\n    -cameras 1\\n    -controlVertices 1\\n    -hulls 1\\n    -grid 1\\n    -imagePlane 1\\n    -joints 1\\n    -ikHandles 1\\n    -deformers 1\\n    -dynamics 1\\n    -particleInstancers 1\\n    -fluids 1\\n    -hairSystems 1\\n    -follicles 1\\n    -nCloths 1\\n    -nParticles 1\\n    -nRigids 1\\n    -dynamicConstraints 1\\n    -locators 1\\n    -manipulators 1\\n    -pluginShapes 1\\n    -dimensions 1\\n    -handles 1\\n    -pivots 1\\n    -textures 1\\n    -strokes 1\\n    -motionTrails 1\\n    -clipGhosts 1\\n    -greasePencils 1\\n    -shadows 0\\n    -captureSequenceNumber -1\\n    -width 1173\\n    -height 686\\n    -sceneRenderFilter 0\\n    $editorName;\\nmodelEditor -e -viewSelected 0 $editorName;\\nmodelEditor -e \\n    -pluginObjects \\\"gpuCacheDisplayFilter\\\" 1 \\n    $editorName\"\n"
		+ "\t\t\t\t\t\"modelPanel -edit -l (localizedPanelLabel(\\\"Persp View\\\")) -mbv $menusOkayInPanels  $panelName;\\n$editorName = $panelName;\\nmodelEditor -e \\n    -cam `findStartUpCamera persp` \\n    -useInteractiveMode 0\\n    -displayLights \\\"default\\\" \\n    -displayAppearance \\\"smoothShaded\\\" \\n    -activeOnly 0\\n    -ignorePanZoom 0\\n    -wireframeOnShaded 0\\n    -headsUpDisplay 1\\n    -holdOuts 1\\n    -selectionHiliteDisplay 1\\n    -useDefaultMaterial 0\\n    -bufferMode \\\"double\\\" \\n    -twoSidedLighting 0\\n    -backfaceCulling 0\\n    -xray 0\\n    -jointXray 0\\n    -activeComponentsXray 0\\n    -displayTextures 1\\n    -smoothWireframe 0\\n    -lineWidth 1\\n    -textureAnisotropic 0\\n    -textureHilight 1\\n    -textureSampling 2\\n    -textureDisplay \\\"modulate\\\" \\n    -textureMaxSize 32768\\n    -fogging 0\\n    -fogSource \\\"fragment\\\" \\n    -fogMode \\\"linear\\\" \\n    -fogStart 0\\n    -fogEnd 100\\n    -fogDensity 0.1\\n    -fogColor 0.5 0.5 0.5 1 \\n    -depthOfFieldPreview 1\\n    -maxConstantTransparency 1\\n    -rendererName \\\"vp2Renderer\\\" \\n    -objectFilterShowInHUD 1\\n    -isFiltered 0\\n    -colorResolution 256 256 \\n    -bumpResolution 512 512 \\n    -textureCompression 0\\n    -transparencyAlgorithm \\\"frontAndBackCull\\\" \\n    -transpInShadows 0\\n    -cullingOverride \\\"none\\\" \\n    -lowQualityLighting 0\\n    -maximumNumHardwareLights 1\\n    -occlusionCulling 0\\n    -shadingModel 0\\n    -useBaseRenderer 0\\n    -useReducedRenderer 0\\n    -smallObjectCulling 0\\n    -smallObjectThreshold -1 \\n    -interactiveDisableShadows 0\\n    -interactiveBackFaceCull 0\\n    -sortTransparent 1\\n    -controllers 1\\n    -nurbsCurves 1\\n    -nurbsSurfaces 1\\n    -polymeshes 1\\n    -subdivSurfaces 1\\n    -planes 1\\n    -lights 1\\n    -cameras 1\\n    -controlVertices 1\\n    -hulls 1\\n    -grid 1\\n    -imagePlane 1\\n    -joints 1\\n    -ikHandles 1\\n    -deformers 1\\n    -dynamics 1\\n    -particleInstancers 1\\n    -fluids 1\\n    -hairSystems 1\\n    -follicles 1\\n    -nCloths 1\\n    -nParticles 1\\n    -nRigids 1\\n    -dynamicConstraints 1\\n    -locators 1\\n    -manipulators 1\\n    -pluginShapes 1\\n    -dimensions 1\\n    -handles 1\\n    -pivots 1\\n    -textures 1\\n    -strokes 1\\n    -motionTrails 1\\n    -clipGhosts 1\\n    -greasePencils 1\\n    -shadows 0\\n    -captureSequenceNumber -1\\n    -width 1173\\n    -height 686\\n    -sceneRenderFilter 0\\n    $editorName;\\nmodelEditor -e -viewSelected 0 $editorName;\\nmodelEditor -e \\n    -pluginObjects \\\"gpuCacheDisplayFilter\\\" 1 \\n    $editorName\"\n"
		+ "\t\t\t\t$configName;\n\n            setNamedPanelLayout (localizedPanelLabel(\"Current Layout\"));\n        }\n\n        panelHistory -e -clear mainPanelHistory;\n        sceneUIReplacement -clear;\n\t}\n\n\ngrid -spacing 5 -size 12 -divisions 5 -displayAxes yes -displayGridLines yes -displayDivisionLines yes -displayPerspectiveLabels no -displayOrthographicLabels no -displayAxesBold yes -perspectiveLabelPosition axis -orthographicLabelPosition edge;\nviewManip -drawCompass 0 -compassAngle 0 -frontParameters \"\" -homeParameters \"\" -selectionLockParameters \"\";\n}\n");
	setAttr ".st" 3;
createNode script -n "sceneConfigurationScriptNode";
	rename -uid "45548CFD-4E3E-5C86-0D1F-E2B49DE3C1E0";
	setAttr ".b" -type "string" "playbackOptions -min 1 -max 120 -ast 1 -aet 200 ";
	setAttr ".st" 6;
createNode expression -n "expression1";
	rename -uid "A8EE8A93-4BDC-0143-0F1C-67B0F1FD598C";
	setAttr -k on ".nds";
	setAttr ".ixp" -type "string" (
		"/**\n * A simple, portable pseudo-random number generator.\n * @brief Generates a deterministic float between 0.0 and 1.0 from an integer seed.\n * @param $seed An integer used to generate the random number.\n * @return A pseudo-random float between 0.0 and 1.0.\n */\nproc float portable_rand(int $seed) {\n    // A common technique for a simple hash-like random number.\n    // The large prime numbers are used to create a chaotic, unpredictable result.\n    float $val = (float)$seed * 12.9898;\n    \n    // --- FIX for float precision on GPUs and other platforms ---\n    // By using the mathematical property sin(x) = sin(x mod 2p), we can wrap the\n    // input to sin() into a high-precision range, ensuring the result\n    // remains stable and correct indefinitely.\n    float $twoPi = 6.28318530718;\n    $val = $val % $twoPi;\n\n\n    float $result = sin($val) * 43758.5453;\n    \n    // MEL's equivalent of frac()\n    return $result - floor($result);\n}\n\n// Generates a persistent random value that holds for a calculated duration.\nproc float fpsr_sm(\n"
		+ "    int $frame, int $minHold, int $maxHold,\n    int $reseedInterval, int $seedInner, int $seedOuter, int $finalRandSwitch)\n{\n    // --- 1. Calculate the random hold duration ---\n    if ($reseedInterval < 1) { $reseedInterval = 1; } // Prevent division by zero.\n\n\n    float $rand_for_duration = portable_rand($seedInner + $frame - ($frame % $reseedInterval));\n    int $holdDuration = (int)floor($minHold + $rand_for_duration * ($maxHold - $minHold));\n\n\n    if ($holdDuration < 1) { $holdDuration = 1; } // Prevent division by zero.\n\n\n    // --- 2. Generate the stable integer \"state\" for the hold period ---\n    int $held_integer_state = ($seedOuter + $frame) - (($seedOuter + $frame) % $holdDuration);\n\n\n    // --- 3. Use the stable state as a seed for the final random value ---\n    float $fpsr_output = 0.0;\n    if ($finalRandSwitch) {\n        $fpsr_output = portable_rand($held_integer_state);\n    } else {\n        $fpsr_output = $held_integer_state;\n    }\n    return $fpsr_output;\n}\n\n\n// --- Main Expression Logic (SM) ---\n"
		+ "\n\n// Parameters\nint $minHoldFrames = 16; // probable minimum held period\nint $maxHoldFrames = 24; // maximum held period before cycling\nint $reseedFrames = 9; // inner mod cycle timing\nint $offsetInner = -41; // offsets the inner frame\nint $offsetOuter = 23; // offsets the outer frame\nint $finalRandSwitch = 1; // 1 to apply the final randomisation step, 0 to skip it\n\n\n// Call the FPS-R:SM function\nfloat $randVal =\n    fpsr_sm(\n        frame, $minHoldFrames, $maxHoldFrames,\n        $reseedFrames, $offsetInner, $offsetOuter, $finalRandSwitch);\n\n/*\n// Optional: check if the value has changed from the previous frame\nfloat $randVal_previous =\n    fpsr_sm(\n        (frame-1), $minHoldFrames, $maxHoldFrames,\n        $reseedFrames, $offsetInner, $offsetOuter, $finalRandSwitch);\nint $changed = 0;\nif ($randVal != $randVal_previous) {\n    $changed = 1;\n}\n*/\n\n// Assign FPS-R SM output \n.O[0] = $randVal;");
createNode expression -n "expression2";
	rename -uid "CEAF346D-4886-22CB-81EA-F8AA7627EA77";
	setAttr -k on ".nds";
	setAttr ".ixp" -type "string" (
		"/**\n * A simple, portable pseudo-random number generator.\n * @brief Generates a deterministic float between 0.0 and 1.0 from an integer seed.\n * @param $seed An integer used to generate the random number.\n * @return A pseudo-random float between 0.0 and 1.0.\n */\nproc float portable_rand(int $seed) {\n    // A common technique for a simple hash-like random number.\n    // The large prime numbers are used to create a chaotic, unpredictable result.\n    float $val = (float)$seed * 12.9898;\n    \n    // --- FIX for float precision on GPUs and other platforms ---\n    // By using the mathematical property sin(x) = sin(x mod 2p), we can wrap the\n    // input to sin() into a high-precision range, ensuring the result\n    // remains stable and correct indefinitely.\n    float $twoPi = 6.28318530718;\n    $val = $val % $twoPi;\n\n\n    float $result = sin($val) * 43758.5453;\n    \n    // MEL's equivalent of frac()\n    return $result - floor($result);\n}\n\n// Generates a persistent random value that holds for a calculated duration.\nproc float fpsr_sm(\n"
		+ "    int $frame, int $minHold, int $maxHold,\n    int $reseedInterval, int $seedInner, int $seedOuter, int $finalRandSwitch)\n{\n    // --- 1. Calculate the random hold duration ---\n    if ($reseedInterval < 1) { $reseedInterval = 1; } // Prevent division by zero.\n\n\n    float $rand_for_duration = portable_rand($seedInner + $frame - ($frame % $reseedInterval));\n    int $holdDuration = (int)floor($minHold + $rand_for_duration * ($maxHold - $minHold));\n\n\n    if ($holdDuration < 1) { $holdDuration = 1; } // Prevent division by zero.\n\n\n    // --- 2. Generate the stable integer \"state\" for the hold period ---\n    int $held_integer_state = ($seedOuter + $frame) - (($seedOuter + $frame) % $holdDuration);\n\n\n    // --- 3. Use the stable state as a seed for the final random value ---\n    float $fpsr_output = 0.0;\n    if ($finalRandSwitch) {\n        $fpsr_output = portable_rand($held_integer_state);\n    } else {\n        $fpsr_output = $held_integer_state;\n    }\n    return $fpsr_output;\n}\n\n\n// --- Main Expression Logic (SM) ---\n"
		+ "\n\n// Parameters\nint $minHoldFrames = 16; // probable minimum held period\nint $maxHoldFrames = 24; // maximum held period before cycling\nint $reseedFrames = 9; // inner mod cycle timing\nint $offsetInner = -21; // offsets the inner frame\nint $offsetOuter = 23; // offsets the outer frame\nint $finalRandSwitch = 1; // 1 to apply the final randomisation step, 0 to skip it\n\n\n// Call the FPS-R:SM function\nfloat $randVal =\n    fpsr_sm(\n        frame, $minHoldFrames, $maxHoldFrames,\n        $reseedFrames, $offsetInner, $offsetOuter, $finalRandSwitch);\n\n/*\n// Optional: check if the value has changed from the previous frame\nfloat $randVal_previous =\n    fpsr_sm(\n        (frame-1), $minHoldFrames, $maxHoldFrames,\n        $reseedFrames, $offsetInner, $offsetOuter, $finalRandSwitch);\nint $changed = 0;\nif ($randVal != $randVal_previous) {\n    $changed = 1;\n}\n*/\n\n// Assign FPS-R SM output \n.O[0] = $randVal;");
createNode expression -n "expression3";
	rename -uid "6F23D66F-42A5-5C4E-5163-B59F826AD77B";
	setAttr -k on ".nds";
	setAttr ".ixp" -type "string" (
		"// A simple, portable pseudo-random number generator.\n/**\n * A simple, portable pseudo-random number generator.\n * @brief Generates a deterministic float between 0.0 and 1.0 from an integer seed.\n * @param $seed An integer used to generate the random number.\n * @return A pseudo-random float between 0.0 and 1.0.\n */\nproc float portable_rand(int $seed) {\n    // A common technique for a simple hash-like random number.\n    // The large prime numbers are used to create a chaotic, unpredictable result.\n    float $val = (float)$seed * 12.9898;\n    \n    // --- FIX for float precision on GPUs and other platforms ---\n    // By using the mathematical property sin(x) = sin(x mod 2p), we can wrap the\n    // input to sin() into a high-precision range, ensuring the result\n    // remains stable and correct indefinitely.\n    float $twoPi = 6.28318530718;\n    $val = $val % $twoPi;\n\n\n    float $result = sin($val) * 43758.5453;\n    \n    // MEL's equivalent of frac()\n    return $result - floor($result);\n}\n\n// ============================================================================\n"
		+ "// EXPRESSION 2: Quantised Switching (QS) - Self-Contained\n// ============================================================================\n\n\n// --- Start of Quantised Switching (QS) Expression ---\n\n\n// A simple, portable pseudo-random number generator.\nproc float portable_rand(int $seed) {\n    float $result = sin((float)$seed * 12.9898) * 43758.5453;\n    // Use `$result - floor($result)` as a replacement for frac() for older Maya versions.\n    // This ensures the result is always a positive value in the [0, 1) range.\n    return $result - floor($result);\n}\n\n\n// Generates a flickering, quantised value by switching between two sine wave streams.\nproc float fpsr_qs(\n    int $frame, float $baseWaveFreq, float $stream2FreqMult,\n    int $quantLevelsMinMax[], int $streamsOffset[],\n    int $streamSwitchDur, int $stream1QuantDur, int $stream2QuantDur,\n    int $finalRandSwitch)\n{\n    // --- 1. Set default durations if not provided ---\n    if ($streamSwitchDur < 1) { $streamSwitchDur = (int)floor((1.0 / $baseWaveFreq) * 0.76); }\n"
		+ "    if ($stream1QuantDur < 1) { $stream1QuantDur = (int)floor((1.0 / $baseWaveFreq) * 1.2); }\n    if ($stream2QuantDur < 1) { $stream2QuantDur = (int)floor((1.0 / $baseWaveFreq) * 0.9); }\n    if ($streamSwitchDur < 1) { $streamSwitchDur = 1; }\n    if ($stream1QuantDur < 1) { $stream1QuantDur = 1; }\n    if ($stream2QuantDur < 1) { $stream2QuantDur = 1; }\n\n\n    // --- 2. Calculate quantisation levels for each stream ---\n    int $s1_quant_level;\n    if (($streamsOffset[0] + $frame) % $stream1QuantDur < $stream1QuantDur * 0.5) {\n        $s1_quant_level = $quantLevelsMinMax[0];\n    } else {\n        $s1_quant_level = $quantLevelsMinMax[1];\n    }\n\n\n    int $s2_quant_level;\n    float $STREAM2_QUANT_RATIO_MIN = 1.24;\n    float $STREAM2_QUANT_RATIO_MAX = 0.66;\n    if (($streamsOffset[1] + $frame) % $stream2QuantDur < $stream2QuantDur * 0.5) {\n        $s2_quant_level = (int)floor($quantLevelsMinMax[0] * $STREAM2_QUANT_RATIO_MIN);\n    } else {\n        $s2_quant_level = (int)floor($quantLevelsMinMax[1] * $STREAM2_QUANT_RATIO_MAX);\n"
		+ "    }\n    if ($s1_quant_level < 1) { $s1_quant_level = 1; }\n    if ($s2_quant_level < 1) { $s2_quant_level = 1; }\n\n\n    // --- 3. Generate the two quantised sine wave streams ---\n    float $STREAM2_DEFAULT_FREQ_MULT = 3.7;\n    if ($stream2FreqMult < 0) { $stream2FreqMult = $STREAM2_DEFAULT_FREQ_MULT; }\n\n\n    float $stream1 = floor(sin((float)($streamsOffset[0] + $frame) * $baseWaveFreq) * $s1_quant_level) / $s1_quant_level;\n    float $stream2 = floor(sin((float)($streamsOffset[1] + $frame) * $baseWaveFreq * $stream2FreqMult) * $s2_quant_level) / $s2_quant_level;\n\n\n    // --- 4. Switch between the two streams ---\n    float $active_stream_val = 0.0;\n    if (($frame % $streamSwitchDur) < $streamSwitchDur / 2) {\n        $active_stream_val = $stream1;\n    } else {\n        $active_stream_val = $stream2;\n    }\n\n\n    // --- 5. Hash the final output to create a random-looking value ---\n    float $fpsr_output = 0.0;\n    if ($finalRandSwitch) {\n        $fpsr_output = portable_rand((int)($active_stream_val * 100000.0));\n    } else {\n"
		+ "        $fpsr_output =  0.5 * $active_stream_val + 0.5; // Scale from [-1, 1] to [0, 1]\n    }\n    return $fpsr_output;\n}\n\n\n// --- Main Expression Logic (QS) ---\n\n\n// Parameters\nfloat $baseWaveFreq = 0.008; // Base frequency for the modulation wave of stream 1\nfloat $stream2freqMult = 1.7; // Multiplier for the second stream's frequency\nint $quantLevelsMinMax[] = {5, 11}; // Min, Max quantisation levels for the two streams\nint $streamsOffset[] = {111, -46}; // Offset for the two streams\nint $streamSwitchDur = 37; // Duration for switching streams in frames\nint $stream1QuantDur = 36; // Duration for the first stream's quantisation switch cycle in frames\nint $stream2QuantDur = 30; // Duration for the second stream's quantisation switch cycle in frames\nint $finalRandSwitch = 0; // 1 to apply the final randomisation step, 0 to skip it\n\n\n// Call the FPS-R:QS function\nfloat $randVal = fpsr_qs(\n    frame, $baseWaveFreq, $stream2freqMult, $quantLevelsMinMax,\n    $streamsOffset, $streamSwitchDur, $stream1QuantDur, $stream2QuantDur, $finalRandSwitch);\n"
		+ "\n\n/*\n// Optional: check if the value has changed from the previous frame\nfloat $randVal_previous = fpsr_qs(\n    (frame - 1), $baseWaveFreq, $stream2freqMult, $quantLevelsMinMax,\n    $streamsOffset, $streamSwitchDur, $stream1QuantDur, $stream2QuantDur, $finalRandSwitch);\nint $changed = 0;\nif ($randVal != $randVal_previous) {\n    $changed = 1;\n}\n*/\n\n\n// ASSIGN THE FINAL VALUE to your object's attribute.\n// ** IMPORTANT: CHANGE \"pCube1.visibility\" to your target attribute! **\n// pCube1.visibility = ($randVal > 0.5);\n.O[0] = $randVal;\n\n\n// --- End of Quantised Switching (QS) Expression ---");
createNode expression -n "expression4";
	rename -uid "C3CACE88-4BAC-D945-7ECA-0A95EBFD2210";
	setAttr -k on ".nds";
	setAttr ".ixp" -type "string" (
		"// A simple, portable pseudo-random number generator.\n/**\n * A simple, portable pseudo-random number generator.\n * @brief Generates a deterministic float between 0.0 and 1.0 from an integer seed.\n * @param $seed An integer used to generate the random number.\n * @return A pseudo-random float between 0.0 and 1.0.\n */\nproc float portable_rand(int $seed) {\n    // A common technique for a simple hash-like random number.\n    // The large prime numbers are used to create a chaotic, unpredictable result.\n    float $val = (float)$seed * 12.9898;\n    \n    // --- FIX for float precision on GPUs and other platforms ---\n    // By using the mathematical property sin(x) = sin(x mod 2p), we can wrap the\n    // input to sin() into a high-precision range, ensuring the result\n    // remains stable and correct indefinitely.\n    float $twoPi = 6.28318530718;\n    $val = $val % $twoPi;\n\n\n    float $result = sin($val) * 43758.5453;\n    \n    // MEL's equivalent of frac()\n    return $result - floor($result);\n}\n\n// ============================================================================\n"
		+ "// EXPRESSION 2: Quantised Switching (QS) - Self-Contained\n// ============================================================================\n\n\n// --- Start of Quantised Switching (QS) Expression ---\n\n\n// A simple, portable pseudo-random number generator.\nproc float portable_rand(int $seed) {\n    float $result = sin((float)$seed * 12.9898) * 43758.5453;\n    // Use `$result - floor($result)` as a replacement for frac() for older Maya versions.\n    // This ensures the result is always a positive value in the [0, 1) range.\n    return $result - floor($result);\n}\n\n\n// Generates a flickering, quantised value by switching between two sine wave streams.\nproc float fpsr_qs(\n    int $frame, float $baseWaveFreq, float $stream2FreqMult,\n    int $quantLevelsMinMax[], int $streamsOffset[],\n    int $streamSwitchDur, int $stream1QuantDur, int $stream2QuantDur,\n    int $finalRandSwitch)\n{\n    // --- 1. Set default durations if not provided ---\n    if ($streamSwitchDur < 1) { $streamSwitchDur = (int)floor((1.0 / $baseWaveFreq) * 0.76); }\n"
		+ "    if ($stream1QuantDur < 1) { $stream1QuantDur = (int)floor((1.0 / $baseWaveFreq) * 1.2); }\n    if ($stream2QuantDur < 1) { $stream2QuantDur = (int)floor((1.0 / $baseWaveFreq) * 0.9); }\n    if ($streamSwitchDur < 1) { $streamSwitchDur = 1; }\n    if ($stream1QuantDur < 1) { $stream1QuantDur = 1; }\n    if ($stream2QuantDur < 1) { $stream2QuantDur = 1; }\n\n\n    // --- 2. Calculate quantisation levels for each stream ---\n    int $s1_quant_level;\n    if (($streamsOffset[0] + $frame) % $stream1QuantDur < $stream1QuantDur * 0.5) {\n        $s1_quant_level = $quantLevelsMinMax[0];\n    } else {\n        $s1_quant_level = $quantLevelsMinMax[1];\n    }\n\n\n    int $s2_quant_level;\n    float $STREAM2_QUANT_RATIO_MIN = 1.24;\n    float $STREAM2_QUANT_RATIO_MAX = 0.66;\n    if (($streamsOffset[1] + $frame) % $stream2QuantDur < $stream2QuantDur * 0.5) {\n        $s2_quant_level = (int)floor($quantLevelsMinMax[0] * $STREAM2_QUANT_RATIO_MIN);\n    } else {\n        $s2_quant_level = (int)floor($quantLevelsMinMax[1] * $STREAM2_QUANT_RATIO_MAX);\n"
		+ "    }\n    if ($s1_quant_level < 1) { $s1_quant_level = 1; }\n    if ($s2_quant_level < 1) { $s2_quant_level = 1; }\n\n\n    // --- 3. Generate the two quantised sine wave streams ---\n    float $STREAM2_DEFAULT_FREQ_MULT = 3.7;\n    if ($stream2FreqMult < 0) { $stream2FreqMult = $STREAM2_DEFAULT_FREQ_MULT; }\n\n\n    float $stream1 = floor(sin((float)($streamsOffset[0] + $frame) * $baseWaveFreq) * $s1_quant_level) / $s1_quant_level;\n    float $stream2 = floor(sin((float)($streamsOffset[1] + $frame) * $baseWaveFreq * $stream2FreqMult) * $s2_quant_level) / $s2_quant_level;\n\n\n    // --- 4. Switch between the two streams ---\n    float $active_stream_val = 0.0;\n    if (($frame % $streamSwitchDur) < $streamSwitchDur / 2) {\n        $active_stream_val = $stream1;\n    } else {\n        $active_stream_val = $stream2;\n    }\n\n\n    // --- 5. Hash the final output to create a random-looking value ---\n    float $fpsr_output = 0.0;\n    if ($finalRandSwitch) {\n        $fpsr_output = portable_rand((int)($active_stream_val * 100000.0));\n    } else {\n"
		+ "        $fpsr_output = 0.5 * $active_stream_val + 0.5; // Scale from [-1, 1] to [0, 1]\n    }\n    return $fpsr_output;\n}\n\n\n// --- Main Expression Logic (QS) ---\n\n\n// Parameters\nfloat $baseWaveFreq = 0.008; // Base frequency for the modulation wave of stream 1\nfloat $stream2freqMult = 1.7; // Multiplier for the second stream's frequency\nint $quantLevelsMinMax[] = {3, 11}; // Min, Max quantisation levels for the two streams\nint $streamsOffset[] = {0, -76}; // Offset for the two streams\nint $streamSwitchDur = 34; // Duration for switching streams in frames\nint $stream1QuantDur = 31; // Duration for the first stream's quantisation switch cycle in frames\nint $stream2QuantDur = 39; // Duration for the second stream's quantisation switch cycle in frames\nint $finalRandSwitch = 1; // 1 to apply the final randomisation step, 0 to skip it\n\n\n// Call the FPS-R:QS function\nfloat $randVal = fpsr_qs(\n    frame, $baseWaveFreq, $stream2freqMult, $quantLevelsMinMax,\n    $streamsOffset, $streamSwitchDur, $stream1QuantDur, $stream2QuantDur, $finalRandSwitch);\n"
		+ "\n\n/*\n// Optional: check if the value has changed from the previous frame\nfloat $randVal_previous = fpsr_qs(\n    (frame - 1), $baseWaveFreq, $stream2freqMult, $quantLevelsMinMax,\n    $streamsOffset, $streamSwitchDur, $stream1QuantDur, $stream2QuantDur, $finalRandSwitch);\nint $changed = 0;\nif ($randVal != $randVal_previous) {\n    $changed = 1;\n}\n*/\n\n\n// ASSIGN THE FINAL VALUE to your object's attribute.\n// ** IMPORTANT: CHANGE \"pCube1.visibility\" to your target attribute! **\n// pCube1.visibility = ($randVal > 0.5);\n.O[0] = $randVal;\n\n\n// --- End of Quantised Switching (QS) Expression ---");
createNode type -n "type1";
	rename -uid "B6A62804-490F-2B0B-2625-6EB0265F8395";
	setAttr ".solidsPerCharacter" -type "doubleArray" 7 1 1 1 1 1 1 1 ;
	setAttr ".solidsPerWord" -type "doubleArray" 2 5 2 ;
	setAttr ".solidsPerLine" -type "doubleArray" 2 5 2 ;
	setAttr ".vertsPerChar" -type "doubleArray" 7 10 54 138 142 181 265 278 ;
	setAttr ".characterBoundingBoxesMax" -type "vectorArray" 7 0.41080165894563536
		 0.59723146296729723 0 0.8885537454904604 0.59723146296729723 0 1.3047603575651312
		 0.61216534070732187 0 1.7651653132162801 0.26834711752647211 0 2.3405537565877617
		 0.59723146296729723 0 0.40393391128413936 -0.38783465929267824 0 1.0814463087349884
		 -0.40276853703270277 0 ;
	setAttr ".characterBoundingBoxesMin" -type "vectorArray" 7 0.077074381930768987
		 0 0 0.52335537366630613 0 0 0.94117355740759989 -0.014925620772621849 0 1.406826448834632
		 0.21911570651472109 0 1.9035206629225045 0 0 0.040347111126608104 -1.0149256207726218
		 0 0.52335537366630613 -1 0 ;
	setAttr ".manipulatorPivots" -type "vectorArray" 7 0.077074381930768987 0 0 0.52335537366630613
		 0 0 0.94117355740759989 -0.014925620772621849 0 1.406826448834632 0.21911570651472109
		 0 1.9035206629225045 0 0 0.040347111126608104 -1.0149256207726218 0 0.52335537366630613
		 -1 0 ;
	setAttr ".holeInfo" -type "Int32Array" 6 1 19 35 4 15
		 166 ;
	setAttr ".numberOfShells" 7;
	setAttr ".textInput" -type "string" "46 50 53 2D 52 0A 53 4D";
	setAttr ".currentFont" -type "string" "Lucida Sans Unicode";
	setAttr ".currentStyle" -type "string" "Regular";
	setAttr ".manipulatorPositionsPP" -type "vectorArray" 23 0 0 0 0 0 0 0 0 0 0
		 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
		 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 ;
	setAttr ".manipulatorWordPositionsPP" -type "vectorArray" 2 0 0 0 0 0 0 ;
	setAttr ".manipulatorLinePositionsPP" -type "vectorArray" 2 0 0 0 0 0 0 ;
	setAttr ".manipulatorRotationsPP" -type "vectorArray" 23 0 0 0 0 0 0 0 0 0 0
		 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
		 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 ;
	setAttr ".manipulatorWordRotationsPP" -type "vectorArray" 2 0 0 0 0 0 0 ;
	setAttr ".manipulatorLineRotationsPP" -type "vectorArray" 2 0 0 0 0 0 0 ;
	setAttr ".manipulatorScalesPP" -type "vectorArray" 23 0 0 0 0 0 0 0 0 0 0 0 0 0
		 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
		 0 0 0 0 0 0 0 0 0 0 0 0 0 0 ;
	setAttr ".manipulatorWordScalesPP" -type "vectorArray" 2 0 0 0 0 0 0 ;
	setAttr ".manipulatorLineScalesPP" -type "vectorArray" 2 0 0 0 0 0 0 ;
	setAttr ".alignmentAdjustments" -type "doubleArray" 2 1.1317396873284964 0.52054959880419016 ;
	setAttr ".manipulatorMode" 0;
	setAttr ".fontSize" 1;
	setAttr ".alignmentMode" 2;
createNode typeExtrude -n "typeExtrude1";
	rename -uid "8EA11453-403F-6871-93AD-D5B840E738CA";
	addAttr -s false -ci true -h true -sn "typeMessage" -ln "typeMessage" -at "message";
	setAttr ".enEx" no;
	setAttr -s 4 ".exc[0:3]"  0 0.5 0.333 0.5 0.66600001 0.5 1 0.5;
	setAttr -s 4 ".fbc[0:3]"  0 1 0.5 1 1 0.5 1 0;
	setAttr -s 4 ".bbc[0:3]"  0 1 0.5 1 1 0.5 1 0;
	setAttr ".capComponents" -type "componentList" 1 "f[0:6]";
	setAttr ".bevelComponents" -type "componentList" 0;
	setAttr ".extrusionComponents" -type "componentList" 0;
	setAttr -s 7 ".charGroupId";
createNode groupId -n "groupid1";
	rename -uid "7E65FBCF-4057-0215-FF98-C087E420E52A";
createNode groupId -n "groupid2";
	rename -uid "C8A9BDCA-4FB3-042A-B5BE-59B436EF3654";
createNode groupId -n "groupid3";
	rename -uid "00E94B96-4E75-DB83-1FD7-3AA07687FB3E";
createNode blinn -n "typeBlinn";
	rename -uid "44B638F9-43C4-ED2B-44AE-ED83F8307856";
	setAttr ".c" -type "float3" 1 1 1 ;
createNode shadingEngine -n "typeBlinnSG";
	rename -uid "7521A927-4B3F-409D-29EA-66AE5B7EC2C2";
	setAttr ".ihi" 0;
	setAttr -s 3 ".dsm";
	setAttr ".ro" yes;
createNode materialInfo -n "materialInfo1";
	rename -uid "C5E8B717-47BF-712C-E2EE-14B1F1C0EA3D";
createNode vectorAdjust -n "vectorAdjust1";
	rename -uid "01EA2C0B-4AD4-72A9-3E4D-4384A337D815";
	setAttr ".extrudeDistanceScalePP" -type "doubleArray" 0 ;
	setAttr ".boundingBoxes" -type "vectorArray" 14 0.077074378728866577 0 0 0.07707437872920031
		 5.972314476966858e-13 0 0.52335536479949951 0 0 0.52335536479986466 5.972314476966858e-13
		 0 0.94117355346679688 -0.014925620518624783 0 0.94117355346716047 -0.014925620517997692
		 0 1.4068264961242676 0.21911570429801941 0 1.406826496124626 0.21911570429806865
		 0 1.9035207033157349 0 0 1.9035207033161718 5.972314476966858e-13 0 0.040347110480070114
		 -1.0149255990982056 0 0.040347110480433698 -1.0149255990975785 0 0.52335536479949951
		 -1 0 0.52335536480005762 -0.99999999999940281 0 ;
createNode polySoftEdge -n "polySoftEdge1";
	rename -uid "65051736-4769-E2EA-A930-1A941CCBD9B7";
	setAttr ".uopa" yes;
	setAttr ".ics" -type "componentList" 1 "e[*]";
	setAttr ".ix" -type "matrix" 1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1;
createNode polyRemesh -n "polyRemesh1";
	rename -uid "78892960-48CB-EEB8-31C5-198AF223E6DA";
	addAttr -s false -ci true -h true -sn "typeMessage" -ln "typeMessage" -at "message";
	setAttr ".nds" 1;
	setAttr ".ix" -type "matrix" 1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1;
	setAttr ".tsb" no;
	setAttr ".ipt" 0;
createNode polyAutoProj -n "polyAutoProj1";
	rename -uid "93C61006-49A0-AE86-1FDA-41AFDB1284B0";
	setAttr ".uopa" yes;
	setAttr ".ics" -type "componentList" 1 "f[*]";
	setAttr ".ix" -type "matrix" 1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1;
	setAttr ".ps" 0.20000000298023224;
	setAttr ".dl" yes;
createNode shellDeformer -n "shellDeformer1";
	rename -uid "A04D4D05-4A91-884C-2302-1A8E3CF3164D";
	addAttr -s false -ci true -h true -sn "typeMessage" -ln "typeMessage" -at "message";
createNode groupId -n "groupId1";
	rename -uid "04F834F6-4BB6-9EF8-B2F1-6385442CCF16";
createNode groupId -n "groupId2";
	rename -uid "ED5854B4-4ED7-1C4C-1A1C-50AB44B78664";
createNode groupId -n "groupId3";
	rename -uid "1C00EEC7-4FD5-A0C6-6067-95AE4B859EFF";
createNode groupId -n "groupId4";
	rename -uid "8C421FF0-4B54-D6F4-6701-1B88A94E56F0";
createNode groupId -n "groupId5";
	rename -uid "9B0579AD-4E3D-329A-15C1-A5B1C5287DD6";
createNode groupId -n "groupId6";
	rename -uid "0A5688C3-4409-25CE-16D7-92AF6564D0DD";
createNode groupId -n "groupId7";
	rename -uid "36439409-43FF-23B6-936A-84B79360AB28";
createNode shellDeformer -n "shellDeformer2";
	rename -uid "3B14673A-4E82-2191-5738-6398B2472D06";
	addAttr -s false -ci true -h true -sn "typeMessage" -ln "typeMessage" -at "message";
createNode polyAutoProj -n "polyAutoProj2";
	rename -uid "12E01195-4FD6-1505-35EC-8D9943D97E69";
	setAttr ".uopa" yes;
	setAttr ".ics" -type "componentList" 1 "f[*]";
	setAttr ".ix" -type "matrix" 1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1;
	setAttr ".ps" 0.20000000298023224;
	setAttr ".dl" yes;
createNode polyRemesh -n "polyRemesh2";
	rename -uid "119AC3CC-4D31-EF54-B56F-D293AC6A6790";
	addAttr -s false -ci true -h true -sn "typeMessage" -ln "typeMessage" -at "message";
	setAttr ".nds" 1;
	setAttr ".ix" -type "matrix" 1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1;
	setAttr ".tsb" no;
	setAttr ".ipt" 0;
createNode polySoftEdge -n "polySoftEdge2";
	rename -uid "6B1B31B6-499F-CC8F-E511-E7B380E8A5E7";
	setAttr ".uopa" yes;
	setAttr ".ics" -type "componentList" 1 "e[*]";
	setAttr ".ix" -type "matrix" 1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1;
createNode vectorAdjust -n "vectorAdjust2";
	rename -uid "41C5CEB0-4B9C-1B28-334E-0EA5A975763A";
	setAttr ".extrudeDistanceScalePP" -type "doubleArray" 0 ;
	setAttr ".boundingBoxes" -type "vectorArray" 14 0.077074378728866577 0 0 0.07707437872920031
		 5.972314476966858e-13 0 0.52335536479949951 0 0 0.52335536479986466 5.972314476966858e-13
		 0 0.94117355346679688 -0.014925620518624783 0 0.94117355346716047 -0.014925620517997692
		 0 1.4068264961242676 0.21911570429801941 0 1.406826496124626 0.21911570429806865
		 0 1.9035207033157349 0 0 1.9035207033161718 5.972314476966858e-13 0 0.041966941207647324
		 -1.1194462776184082 0 0.041966941208266349 -1.1194462776176766 0 0.68497520685195923
		 -1.0149255990982056 0 0.68497520685232283 -1.0149255990975785 0 ;
createNode typeExtrude -n "typeExtrude2";
	rename -uid "0505D64E-40C5-33DC-C802-0B9F9CEB3E3C";
	addAttr -s false -ci true -h true -sn "typeMessage" -ln "typeMessage" -at "message";
	setAttr ".enEx" no;
	setAttr -s 4 ".exc[0:3]"  0 0.5 0.333 0.5 0.66600001 0.5 1 0.5;
	setAttr -s 4 ".fbc[0:3]"  0 1 0.5 1 1 0.5 1 0;
	setAttr -s 4 ".bbc[0:3]"  0 1 0.5 1 1 0.5 1 0;
	setAttr ".capComponents" -type "componentList" 1 "f[0:6]";
	setAttr ".bevelComponents" -type "componentList" 0;
	setAttr ".extrusionComponents" -type "componentList" 0;
	setAttr -s 7 ".charGroupId";
createNode type -n "type2";
	rename -uid "D35A4CF3-4B36-A658-34CB-B388D22EFF24";
	setAttr ".solidsPerCharacter" -type "doubleArray" 7 1 1 1 1 1 1 1 ;
	setAttr ".solidsPerWord" -type "doubleArray" 2 5 2 ;
	setAttr ".solidsPerLine" -type "doubleArray" 2 5 2 ;
	setAttr ".vertsPerChar" -type "doubleArray" 7 10 54 138 142 181 254 338 ;
	setAttr ".characterBoundingBoxesMax" -type "vectorArray" 7 0.41080165894563536
		 0.59723146296729723 0 0.8885537454904604 0.59723146296729723 0 1.3047603575651312
		 0.61216534070732187 0 1.7651653132162801 0.26834711752647211 0 2.3405537565877617
		 0.59723146296729723 0 0.66099176328044296 -0.38783465929267824 0 1.0485620104576931
		 -0.38783465929267824 0 ;
	setAttr ".characterBoundingBoxesMin" -type "vectorArray" 7 0.077074381930768987
		 0 0 0.52335537366630613 0 0 0.94117355740759989 -0.014925620772621849 0 1.406826448834632
		 0.21911570651472109 0 1.9035206629225045 0 0 0.04196694271623596 -1.119446289440817
		 0 0.68497521030016184 -1.0149256207726218 0 ;
	setAttr ".manipulatorPivots" -type "vectorArray" 7 0.077074381930768987 0 0 0.52335537366630613
		 0 0 0.94117355740759989 -0.014925620772621849 0 1.406826448834632 0.21911570651472109
		 0 1.9035206629225045 0 0 0.04196694271623596 -1.119446289440817 0 0.68497521030016184
		 -1.0149256207726218 0 ;
	setAttr ".holeInfo" -type "Int32Array" 9 1 19 35 4 15
		 166 5 32 222 ;
	setAttr ".numberOfShells" 7;
	setAttr ".textInput" -type "string" "46 50 53 2D 52 0A 51 53";
	setAttr ".currentFont" -type "string" "Lucida Sans Unicode";
	setAttr ".currentStyle" -type "string" "Regular";
	setAttr ".manipulatorPositionsPP" -type "vectorArray" 23 0 0 0 0 0 0 0 0 0 0
		 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
		 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 ;
	setAttr ".manipulatorWordPositionsPP" -type "vectorArray" 2 0 0 0 0 0 0 ;
	setAttr ".manipulatorLinePositionsPP" -type "vectorArray" 2 0 0 0 0 0 0 ;
	setAttr ".manipulatorRotationsPP" -type "vectorArray" 23 0 0 0 0 0 0 0 0 0 0
		 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
		 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 ;
	setAttr ".manipulatorWordRotationsPP" -type "vectorArray" 2 0 0 0 0 0 0 ;
	setAttr ".manipulatorLineRotationsPP" -type "vectorArray" 2 0 0 0 0 0 0 ;
	setAttr ".manipulatorScalesPP" -type "vectorArray" 23 0 0 0 0 0 0 0 0 0 0 0 0 0
		 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
		 0 0 0 0 0 0 0 0 0 0 0 0 0 0 ;
	setAttr ".manipulatorWordScalesPP" -type "vectorArray" 2 0 0 0 0 0 0 ;
	setAttr ".manipulatorLineScalesPP" -type "vectorArray" 2 0 0 0 0 0 0 ;
	setAttr ".alignmentAdjustments" -type "doubleArray" 2 1.1317396873284964 0.50329753387072862 ;
	setAttr ".manipulatorMode" 0;
	setAttr ".fontSize" 1;
	setAttr ".alignmentMode" 2;
createNode groupId -n "groupid4";
	rename -uid "52FDF639-41D4-ED05-FBEC-D796F85B5DAA";
createNode groupId -n "groupid5";
	rename -uid "5EC312F1-4F50-8B38-EDCD-0FB2750BCF00";
createNode groupId -n "groupid6";
	rename -uid "8694783A-4669-C8A8-F424-FD9C71BB1594";
createNode groupId -n "groupId8";
	rename -uid "DFA43B97-4712-427F-6E3F-D0BFAF02E786";
createNode groupId -n "groupId9";
	rename -uid "D19C5BB6-428F-DEF3-25AA-7E834432F7D4";
createNode groupId -n "groupId10";
	rename -uid "643E833D-4555-CAAC-1F1F-F39D20CCE2F3";
createNode groupId -n "groupId11";
	rename -uid "6A515752-4A38-A790-19EA-86A93D38D004";
createNode groupId -n "groupId12";
	rename -uid "DEA2AAD8-4347-61CA-837E-288104822B41";
createNode groupId -n "groupId13";
	rename -uid "5BC83A5E-4F07-22EC-E125-CE8A7AD2FA90";
createNode groupId -n "groupId14";
	rename -uid "915DA6DE-494F-D3B4-39E6-2DA6BCE99255";
createNode blinn -n "blinn_sm";
	rename -uid "AC76C0EC-4E7D-238E-274E-1181AE9A7851";
createNode shadingEngine -n "blinn1SG";
	rename -uid "A7D5B7AC-46CE-8D2E-E359-85AF5140F902";
	setAttr ".ihi" 0;
	setAttr ".ro" yes;
createNode materialInfo -n "materialInfo2";
	rename -uid "0DA1A778-4703-CE6B-86A4-4ABCD67D0ABB";
createNode blinn -n "blinn_qs";
	rename -uid "869FDB32-411E-7D5D-37EC-C5891F6D9F54";
createNode shadingEngine -n "blinn_qsSG";
	rename -uid "1138872F-4C1A-FEF7-7CB1-5A9A682F4BF1";
	setAttr ".ihi" 0;
	setAttr ".ro" yes;
createNode materialInfo -n "materialInfo3";
	rename -uid "08F75072-4A96-BEE1-3D67-A48DB1F4D27C";
createNode expression -n "expression13";
	rename -uid "8376ED03-463A-C76E-FFEB-A1B7CF82ABA1";
	setAttr -k on ".nds";
	setAttr -s 2 ".in";
	setAttr -s 2 ".in";
	setAttr ".ixp" -type "string" "float $prevY = `getAttr -time (frame-1) FPSR_readme_grp_see_notes_in_attribute_editor|pCube_fpsr_sm.translateY`;\nfloat $prevZ = `getAttr -time (frame-1) FPSR_readme_grp_see_notes_in_attribute_editor|pCube_fpsr_sm.translateZ`;\nfloat $currY = .I[0];\nfloat $currZ = .I[1];\n\nif (($prevY != $currY) || ($prevZ != $currZ)) {\n\t.O[0] = 1;\n} else {\n\t.O[0] = 0;\n}";
createNode colorCondition -n "colorCondition1";
	rename -uid "4F35AD8B-4ADB-2BE6-BE5B-BFB5ACB08D17";
	setAttr "._cb" -type "float3" 0.56099999 0.56099999 0.56099999 ;
createNode colorCondition -n "colorCondition2";
	rename -uid "D7A6BE27-41EE-45A3-D46F-2E970C860ACB";
	setAttr "._cb" -type "float3" 0.45100001 0.45100001 0.45100001 ;
createNode expression -n "expression14";
	rename -uid "5F3C16C0-4BDA-368C-6B64-C2ADE833E23F";
	setAttr -k on ".nds";
	setAttr -s 2 ".in";
	setAttr -s 2 ".in";
	setAttr ".ixp" -type "string" "float $prevY = `getAttr -time (frame-1) FPSR_readme_grp_see_notes_in_attribute_editor|pCube_fpsr_qs.translateY`;\nfloat $prevZ = `getAttr -time (frame-1) FPSR_readme_grp_see_notes_in_attribute_editor|pCube_fpsr_qs.translateZ`;\nfloat $currY = .I[0];\nfloat $currZ = .I[1];\n\nif (($prevY != $currY) || ($prevZ != $currZ)) {\n\t.O[0] = 1;\n} else {\n\t.O[0] = 0;\n}";
createNode expression -n "expression15";
	rename -uid "C615C6EA-4671-E6B3-A965-798E08FEA93C";
	setAttr -k on ".nds";
	setAttr -s 2 ".in";
	setAttr -s 2 ".in";
	setAttr ".ixp" -type "string" "float $prevY = `getAttr -time (frame-1) FPSR_readme_grp_see_notes_in_attribute_editor|pCube_fpsr_tm.translateY`;\nfloat $prevZ = `getAttr -time (frame-1) FPSR_readme_grp_see_notes_in_attribute_editor|pCube_fpsr_tm.translateZ`;\nfloat $currY = .I[0];\nfloat $currZ = .I[1];\n\nif (($prevY != $currY) || ($prevZ != $currZ)) {\n\t.O[0] = 1;\n} else {\n\t.O[0] = 0;\n}";
createNode expression -n "expression16";
	rename -uid "20D52387-4288-F005-7563-58993136B483";
	setAttr -k on ".nds";
	setAttr ".ixp" -type "string" (
		"// --- Start of Toggled Modulo (TM) Expression ---\n/**\n * A simple, portable pseudo-random number generator.\n * @brief Generates a deterministic float between 0.0 and 1.0 from an integer seed.\n * @param $seed An integer used to generate the random number.\n * @return A pseudo-random float between 0.0 and 1.0.\n */\nproc float portable_rand(int $seed) {\n    // A common technique for a simple hash-like random number.\n    // The large prime numbers are used to create a chaotic, unpredictable result.\n    float $val = (float)$seed * 12.9898;\n    \n    // --- FIX for float precision on GPUs and other platforms ---\n    // By using the mathematical property sin(x) = sin(x mod 2p), we can wrap the\n    // input to sin() into a high-precision range, ensuring the result\n    // remains stable and correct indefinitely.\n    float $twoPi = 6.28318530718;\n    $val = $val % $twoPi;\n\n\n    float $result = sin($val) * 43758.5453;\n    \n    // MEL's equivalent of frac()\n    return $result - floor($result);\n}\n\n// Generates a persistent value that holds for a rhythmically toggled duration.\n"
		+ "proc float fpsr_tm(\n    int $frame, int $periodA, int $periodB,\n    int $periodSwitch, int $seedInner, int $seedOuter,\n    int $finalRandSwitch)\n{\n    // --- 1. Determine the hold duration by toggling between two periods ---\n    if ($periodSwitch < 1) { $periodSwitch = 1; } // Prevent division by zero.\n\n\n    // The \"inner clock\" is offset by seedInner to de-correlate it from the main frame.\n    int $inner_clock_frame = $seedInner + $frame;\n    \n    int $holdDuration;\n    // The ternary switch: toggle between periodA and periodB at a fixed rhythm.\n    if (($inner_clock_frame % $periodSwitch) < ($periodSwitch * 0.5)) {\n        $holdDuration = $periodA;\n    } else {\n        $holdDuration = $periodB;\n    }\n\n\n    if ($holdDuration < 1) { $holdDuration = 1; } // Prevent division by zero.\n\n\n    // --- 2. Generate the stable integer \"state\" for the hold period ---\n    // The \"outer clock\" is offset by seedOuter to create unique output sequences.\n    int $outer_clock_frame = $seedOuter + $frame;\n    int $held_integer_state = $outer_clock_frame - ($outer_clock_frame % $holdDuration);\n"
		+ "\n\n    // --- 3. Use the stable state as a seed for the final random value (or bypass) ---\n    float $fpsr_output;\n    if ($finalRandSwitch) {\n        // If true, apply the final randomisation hash.\n        $fpsr_output = portable_rand_tm($held_integer_state * 100000.0);\n    } else {\n        // If false, return the raw integer state directly.\n        $fpsr_output = (float)$held_integer_state; \n    }\n    return $fpsr_output;\n}\n\n\n// --- Main Expression Logic (TM) ---\n\n// Parameters\nint $period_A = 10; // The first hold duration\nint $period_B = 25; // The second hold duration\nint $switch_duration = 30; // The toggle happens every 30 frames\nint $offset_inner_tm = 15; // offsets the inner (toggle) clock\nint $offset_outer_tm = 0; // offsets the outer (hold) clock\nint $final_rand_switch_tm = 1; // 1 to apply the final randomisation step, 0 to skip it\n\n\n// Call the FPS-R:TM function\nfloat $randVal = \n    fpsr_tm(\n        frame, $period_A, $period_B, \n        $switch_duration, $offset_inner_tm, $offset_outer_tm, $final_rand_switch_tm);\n"
		+ "\n\n\n// Optional: check if the value has changed from the previous frame\nfloat $randVal_previous = \n    fpsr_tm(\n        (frame - 1), $period_A, $period_B, \n        $switch_duration, $offset_inner_tm, $offset_outer_tm, $final_rand_switch_tm);\nint $changed_tm = 0;\nif ($randVal != $randVal_previous) {\n    $changed_tm = 1; // value has changed from the previous frame\n}\n\n\n\n// ASSIGN THE FINAL VALUE to your object's attribute.\n// ** IMPORTANT: CHANGE \"pCube1.translateX\" to your target attribute! **\n// pCube1.translateX = $randVal;\n\n\n// --- End of Toggled Modulo (TM) Expression ---\n\n// Assign FPS-R TM output \n.O[0] = $randVal;");
createNode expression -n "expression17";
	rename -uid "C229458C-42D2-6DF5-E0EF-139DB51E4959";
	setAttr -k on ".nds";
	setAttr ".ixp" -type "string" (
		"// --- Start of Toggled Modulo (TM) Expression ---\n/**\n * A simple, portable pseudo-random number generator.\n * @brief Generates a deterministic float between 0.0 and 1.0 from an integer seed.\n * @param $seed An integer used to generate the random number.\n * @return A pseudo-random float between 0.0 and 1.0.\n */\nproc float portable_rand(int $seed) {\n    // A common technique for a simple hash-like random number.\n    // The large prime numbers are used to create a chaotic, unpredictable result.\n    float $val = (float)$seed * 12.9898;\n    \n    // --- FIX for float precision on GPUs and other platforms ---\n    // By using the mathematical property sin(x) = sin(x mod 2p), we can wrap the\n    // input to sin() into a high-precision range, ensuring the result\n    // remains stable and correct indefinitely.\n    float $twoPi = 6.28318530718;\n    $val = $val % $twoPi;\n\n\n    float $result = sin($val) * 43758.5453;\n    \n    // MEL's equivalent of frac()\n    return $result - floor($result);\n}\n\n// Generates a persistent value that holds for a rhythmically toggled duration.\n"
		+ "proc float fpsr_tm(\n    int $frame, int $periodA, int $periodB,\n    int $periodSwitch, int $seedInner, int $seedOuter,\n    int $finalRandSwitch)\n{\n    // --- 1. Determine the hold duration by toggling between two periods ---\n    if ($periodSwitch < 1) { $periodSwitch = 1; } // Prevent division by zero.\n\n\n    // The \"inner clock\" is offset by seedInner to de-correlate it from the main frame.\n    int $inner_clock_frame = $seedInner + $frame;\n    \n    int $holdDuration;\n    // The ternary switch: toggle between periodA and periodB at a fixed rhythm.\n    if (($inner_clock_frame % $periodSwitch) < ($periodSwitch * 0.5)) {\n        $holdDuration = $periodA;\n    } else {\n        $holdDuration = $periodB;\n    }\n\n\n    if ($holdDuration < 1) { $holdDuration = 1; } // Prevent division by zero.\n\n\n    // --- 2. Generate the stable integer \"state\" for the hold period ---\n    // The \"outer clock\" is offset by seedOuter to create unique output sequences.\n    int $outer_clock_frame = $seedOuter + $frame;\n    int $held_integer_state = $outer_clock_frame - ($outer_clock_frame % $holdDuration);\n"
		+ "\n\n    // --- 3. Use the stable state as a seed for the final random value (or bypass) ---\n    float $fpsr_output;\n    if ($finalRandSwitch) {\n        // If true, apply the final randomisation hash.\n        $fpsr_output = portable_rand_tm($held_integer_state * 100000.0);\n    } else {\n        // If false, return the raw integer state directly.\n        $fpsr_output = (float)$held_integer_state; \n    }\n    return $fpsr_output;\n}\n\n\n// --- Main Expression Logic (TM) ---\n\n// Parameters\nint $period_A = 10; // The first hold duration\nint $period_B = 25; // The second hold duration\nint $switch_duration = 30; // The toggle happens every 30 frames\nint $offset_inner_tm = 5; // offsets the inner (toggle) clock\nint $offset_outer_tm = -10; // offsets the outer (hold) clock\nint $final_rand_switch_tm = 1; // 1 to apply the final randomisation step, 0 to skip it\n\n\n// Call the FPS-R:TM function\nfloat $randVal = \n    fpsr_tm(\n        frame, $period_A, $period_B, \n        $switch_duration, $offset_inner_tm, $offset_outer_tm, $final_rand_switch_tm);\n"
		+ "\n\n\n// Optional: check if the value has changed from the previous frame\nfloat $randVal_previous = \n    fpsr_tm(\n        (frame - 1), $period_A, $period_B, \n        $switch_duration, $offset_inner_tm, $offset_outer_tm, $final_rand_switch_tm);\nint $changed_tm = 0;\nif ($randVal != $randVal_previous) {\n    $changed_tm = 1; // value has changed from the previous frame\n}\n\n\n\n// ASSIGN THE FINAL VALUE to your object's attribute.\n// ** IMPORTANT: CHANGE \"pCube1.translateX\" to your target attribute! **\n// pCube1.translateX = $randVal;\n\n\n// --- End of Toggled Modulo (TM) Expression ---\n\n// Assign FPS-R TM output \n.O[0] = $randVal;");
createNode polyCube -n "polyCube2";
	rename -uid "97C4CFA2-4BB6-CA51-6E36-CC8C48CFB7B8";
	setAttr ".cuv" 4;
createNode shellDeformer -n "shellDeformer3";
	rename -uid "5FDAEF8F-4184-12F8-B5F2-33BCE1F93932";
	addAttr -s false -ci true -h true -sn "typeMessage" -ln "typeMessage" -at "message";
createNode polyAutoProj -n "polyAutoProj3";
	rename -uid "CBB8A5A1-461A-C783-1D44-E89E7B8BC892";
	setAttr ".uopa" yes;
	setAttr ".ics" -type "componentList" 1 "f[*]";
	setAttr ".ix" -type "matrix" 1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1;
	setAttr ".ps" 0.20000000298023224;
	setAttr ".dl" yes;
createNode polyRemesh -n "polyRemesh3";
	rename -uid "0D162C0E-42AD-4B5F-2E57-A093BF006C03";
	addAttr -s false -ci true -h true -sn "typeMessage" -ln "typeMessage" -at "message";
	setAttr ".nds" 1;
	setAttr ".ix" -type "matrix" 1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1;
	setAttr ".tsb" no;
	setAttr ".ipt" 0;
createNode polySoftEdge -n "polySoftEdge3";
	rename -uid "9AC59D58-42A1-99FE-2997-329A576C8CA1";
	setAttr ".uopa" yes;
	setAttr ".ics" -type "componentList" 1 "e[*]";
	setAttr ".ix" -type "matrix" 1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1;
createNode vectorAdjust -n "vectorAdjust3";
	rename -uid "51B9E654-44B3-2383-544E-369037F44274";
	setAttr ".extrudeDistanceScalePP" -type "doubleArray" 0 ;
	setAttr ".boundingBoxes" -type "vectorArray" 14 0.077074378728866577 0 0 0.07707437872920031
		 5.972314476966858e-13 0 0.52335536479949951 0 0 0.52335536479986466 5.972314476966858e-13
		 0 0.94117355346679688 -0.014925620518624783 0 0.94117355346716047 -0.014925620517997692
		 0 1.4068264961242676 0.21911570429801941 0 1.406826496124626 0.21911570429806865
		 0 1.9035207033157349 0 0 1.9035207033161718 5.972314476966858e-13 0 0.0072561986744403839
		 -1 0 0.007256198674948847 -0.99999999999940281 0 0.59773552417755127 -1 0 0.59773552417810938
		 -0.99999999999940281 0 ;
createNode typeExtrude -n "typeExtrude3";
	rename -uid "7A62C7D3-4365-1FB5-DEB8-A6A2D37BF6BB";
	addAttr -s false -ci true -h true -sn "typeMessage" -ln "typeMessage" -at "message";
	setAttr ".enEx" no;
	setAttr -s 4 ".exc[0:3]"  0 0.5 0.333 0.5 0.66600001 0.5 1 0.5;
	setAttr -s 4 ".fbc[0:3]"  0 1 0.5 1 1 0.5 1 0;
	setAttr -s 4 ".bbc[0:3]"  0 1 0.5 1 1 0.5 1 0;
	setAttr ".capComponents" -type "componentList" 1 "f[0:6]";
	setAttr ".bevelComponents" -type "componentList" 0;
	setAttr ".extrusionComponents" -type "componentList" 0;
	setAttr -s 7 ".charGroupId";
createNode type -n "type3";
	rename -uid "F8BEF80D-4513-8A5B-75E4-E7AEEF86857D";
	setAttr ".solidsPerCharacter" -type "doubleArray" 7 1 1 1 1 1 1 1 ;
	setAttr ".solidsPerWord" -type "doubleArray" 2 5 2 ;
	setAttr ".solidsPerLine" -type "doubleArray" 2 5 2 ;
	setAttr ".vertsPerChar" -type "doubleArray" 7 10 54 138 142 181 189 202 ;
	setAttr ".characterBoundingBoxesMax" -type "vectorArray" 7 0.41080165894563536
		 0.59723146296729723 0 0.8885537454904604 0.59723146296729723 0 1.3047603575651312
		 0.61216534070732187 0 1.7651653132162801 0.26834711752647211 0 2.3405537565877617
		 0.59723146296729723 0 0.51571904332184593 -0.40276853703270277 0 1.1558264740242445
		 -0.40276853703270277 0 ;
	setAttr ".characterBoundingBoxesMin" -type "vectorArray" 7 0.077074381930768987
		 0 0 0.52335537366630613 0 0 0.94117355740759989 -0.014925620772621849 0 1.406826448834632
		 0.21911570651472109 0 1.9035206629225045 0 0 0.0072561985205027687 -1 0 0.59773553895556242
		 -1 0 ;
	setAttr ".manipulatorPivots" -type "vectorArray" 7 0.077074381930768987 0 0 0.52335537366630613
		 0 0 0.94117355740759989 -0.014925620772621849 0 1.406826448834632 0.21911570651472109
		 0 1.9035206629225045 0 0 0.0072561985205027687 -1 0 0.59773553895556242 -1 0 ;
	setAttr ".holeInfo" -type "Int32Array" 6 1 19 35 4 15
		 166 ;
	setAttr ".numberOfShells" 7;
	setAttr ".textInput" -type "string" "46 50 53 2D 52 0A 54 4D";
	setAttr ".currentFont" -type "string" "Lucida Sans Unicode";
	setAttr ".currentStyle" -type "string" "Regular";
	setAttr ".manipulatorPositionsPP" -type "vectorArray" 23 0 0 0 0 0 0 0 0 0 0
		 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
		 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 ;
	setAttr ".manipulatorWordPositionsPP" -type "vectorArray" 2 0 0 0 0 0 0 ;
	setAttr ".manipulatorLinePositionsPP" -type "vectorArray" 2 0 0 0 0 0 0 ;
	setAttr ".manipulatorRotationsPP" -type "vectorArray" 23 0 0 0 0 0 0 0 0 0 0
		 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
		 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 ;
	setAttr ".manipulatorWordRotationsPP" -type "vectorArray" 2 0 0 0 0 0 0 ;
	setAttr ".manipulatorLineRotationsPP" -type "vectorArray" 2 0 0 0 0 0 0 ;
	setAttr ".manipulatorScalesPP" -type "vectorArray" 23 0 0 0 0 0 0 0 0 0 0 0 0 0
		 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
		 0 0 0 0 0 0 0 0 0 0 0 0 0 0 ;
	setAttr ".manipulatorWordScalesPP" -type "vectorArray" 2 0 0 0 0 0 0 ;
	setAttr ".manipulatorLineScalesPP" -type "vectorArray" 2 0 0 0 0 0 0 ;
	setAttr ".alignmentAdjustments" -type "doubleArray" 2 1.1317396873284964 0.57428513775187084 ;
	setAttr ".manipulatorMode" 0;
	setAttr ".fontSize" 1;
	setAttr ".alignmentMode" 2;
createNode groupId -n "groupid7";
	rename -uid "BF0D3D66-4C4C-47BB-19CB-9F89425CF993";
createNode groupId -n "groupid8";
	rename -uid "673C2C79-4C51-FD54-39E5-98B142E727BB";
createNode groupId -n "groupid9";
	rename -uid "07403323-4FEC-77AF-B70A-2B98E4DF4A38";
createNode groupId -n "groupId15";
	rename -uid "C66ECFFB-4B2D-E7FF-7CE6-BD880DF57630";
createNode groupId -n "groupId16";
	rename -uid "3BA8F9D0-42FB-5911-9888-A690496E0D59";
createNode groupId -n "groupId17";
	rename -uid "49B1CCAF-4CC5-1691-4AF5-D2880D73CE9D";
createNode groupId -n "groupId18";
	rename -uid "1DC1AC43-4228-7F05-5031-3B898A38CD19";
createNode groupId -n "groupId19";
	rename -uid "B04815CA-4F91-54A9-4EF8-3482B54D05C1";
createNode groupId -n "groupId20";
	rename -uid "CBCC33D0-470C-1B99-9337-8EBC620F8678";
createNode groupId -n "groupId21";
	rename -uid "CACAE5D2-453E-B00B-EC01-35B9F5A9E8FA";
createNode colorCondition -n "colorCondition3";
	rename -uid "032CC715-4F3B-52EC-BFB3-B389085C2EB0";
	setAttr "._cb" -type "float3" 0.56099999 0.56099999 0.56099999 ;
createNode blinn -n "blinn_tm";
	rename -uid "50A6B3DA-4542-4411-F798-09B69ED66521";
createNode shadingEngine -n "blinn_tmSG";
	rename -uid "2D3BA29F-4EBE-F8AC-EC06-DAB296596682";
	setAttr ".ihi" 0;
	setAttr ".ro" yes;
createNode materialInfo -n "materialInfo4";
	rename -uid "55B3FBC1-4BBC-B67E-3423-F5974C3F1C26";
createNode nodeGraphEditorInfo -n "MayaNodeEditorSavedTabsInfo";
	rename -uid "FCA06936-4C9A-5F91-61E2-43B36F155192";
	setAttr ".tgi[0].tn" -type "string" "Untitled_1";
	setAttr ".tgi[0].vl" -type "double2" 1515.9389652754915 -559.52378729033194 ;
	setAttr ".tgi[0].vh" -type "double2" 2898.3465736022754 135.71428032148472 ;
	setAttr -s 18 ".tgi[0].ni";
	setAttr ".tgi[0].ni[0].x" 2584.28564453125;
	setAttr ".tgi[0].ni[0].y" -798.5714111328125;
	setAttr ".tgi[0].ni[0].nvs" 18304;
	setAttr ".tgi[0].ni[1].x" 1166.443115234375;
	setAttr ".tgi[0].ni[1].y" 82.291893005371094;
	setAttr ".tgi[0].ni[1].nvs" 18306;
	setAttr ".tgi[0].ni[2].x" 2584.28564453125;
	setAttr ".tgi[0].ni[2].y" 14.285714149475098;
	setAttr ".tgi[0].ni[2].nvs" 18304;
	setAttr ".tgi[0].ni[3].x" 1970;
	setAttr ".tgi[0].ni[3].y" 407.14285278320312;
	setAttr ".tgi[0].ni[3].nvs" 18304;
	setAttr ".tgi[0].ni[4].x" 2277.142822265625;
	setAttr ".tgi[0].ni[4].y" 45.714286804199219;
	setAttr ".tgi[0].ni[4].nvs" 18304;
	setAttr ".tgi[0].ni[5].x" 2584.28564453125;
	setAttr ".tgi[0].ni[5].y" -418.57144165039062;
	setAttr ".tgi[0].ni[5].nvs" 18304;
	setAttr ".tgi[0].ni[6].x" 2277.142822265625;
	setAttr ".tgi[0].ni[6].y" 420;
	setAttr ".tgi[0].ni[6].nvs" 18304;
	setAttr ".tgi[0].ni[7].x" 2584.28564453125;
	setAttr ".tgi[0].ni[7].y" -87.142860412597656;
	setAttr ".tgi[0].ni[7].nvs" 18304;
	setAttr ".tgi[0].ni[8].x" 2277.142822265625;
	setAttr ".tgi[0].ni[8].y" 318.57144165039062;
	setAttr ".tgi[0].ni[8].nvs" 18304;
	setAttr ".tgi[0].ni[9].x" 2277.142822265625;
	setAttr ".tgi[0].ni[9].y" -157.14285278320312;
	setAttr ".tgi[0].ni[9].nvs" 18304;
	setAttr ".tgi[0].ni[10].x" 1970;
	setAttr ".tgi[0].ni[10].y" 32.857143402099609;
	setAttr ".tgi[0].ni[10].nvs" 18304;
	setAttr ".tgi[0].ni[11].x" 2584.28564453125;
	setAttr ".tgi[0].ni[11].y" -317.14285278320312;
	setAttr ".tgi[0].ni[11].nvs" 18304;
	setAttr ".tgi[0].ni[12].x" 1970;
	setAttr ".tgi[0].ni[12].y" 134.28572082519531;
	setAttr ".tgi[0].ni[12].nvs" 18304;
	setAttr ".tgi[0].ni[13].x" 2277.142822265625;
	setAttr ".tgi[0].ni[13].y" -55.714286804199219;
	setAttr ".tgi[0].ni[13].nvs" 18306;
	setAttr ".tgi[0].ni[14].x" 1166.443115234375;
	setAttr ".tgi[0].ni[14].y" 183.720458984375;
	setAttr ".tgi[0].ni[14].nvs" 18304;
	setAttr ".tgi[0].ni[15].x" 2584.28564453125;
	setAttr ".tgi[0].ni[15].y" 115.71428680419922;
	setAttr ".tgi[0].ni[15].nvs" 18304;
	setAttr ".tgi[0].ni[16].x" 2584.28564453125;
	setAttr ".tgi[0].ni[16].y" 370;
	setAttr ".tgi[0].ni[16].nvs" 18304;
	setAttr ".tgi[0].ni[17].x" 1963.291748046875;
	setAttr ".tgi[0].ni[17].y" -100.77126312255859;
	setAttr ".tgi[0].ni[17].nvs" 18306;
createNode nodeGraphEditorInfo -n "hyperShadePrimaryNodeEditorSavedTabsInfo";
	rename -uid "E4D9B6F6-4099-2B9D-5EC7-F2BF0E158783";
	setAttr ".tgi[0].tn" -type "string" "Untitled_1";
	setAttr ".tgi[0].vl" -type "double2" -794.04758749500274 -1394.4327176994659 ;
	setAttr ".tgi[0].vh" -type "double2" 247.61903777955095 -272.23388273982033 ;
	setAttr -s 6 ".tgi[0].ni";
	setAttr ".tgi[0].ni[0].x" -64.285713195800781;
	setAttr ".tgi[0].ni[0].y" -442.85714721679688;
	setAttr ".tgi[0].ni[0].nvs" 1923;
	setAttr ".tgi[0].ni[1].x" -678.5714111328125;
	setAttr ".tgi[0].ni[1].y" -895.71429443359375;
	setAttr ".tgi[0].ni[1].nvs" 1923;
	setAttr ".tgi[0].ni[2].x" -678.5714111328125;
	setAttr ".tgi[0].ni[2].y" -465.71429443359375;
	setAttr ".tgi[0].ni[2].nvs" 1923;
	setAttr ".tgi[0].ni[3].x" -64.285713195800781;
	setAttr ".tgi[0].ni[3].y" -872.85711669921875;
	setAttr ".tgi[0].ni[3].nvs" 1923;
	setAttr ".tgi[0].ni[4].x" -371.42855834960938;
	setAttr ".tgi[0].ni[4].y" -872.85711669921875;
	setAttr ".tgi[0].ni[4].nvs" 1923;
	setAttr ".tgi[0].ni[5].x" -371.42855834960938;
	setAttr ".tgi[0].ni[5].y" -442.85714721679688;
	setAttr ".tgi[0].ni[5].nvs" 1923;
select -ne :time1;
	setAttr ".o" 1;
	setAttr ".unw" 1;
select -ne :hardwareRenderingGlobals;
	setAttr ".otfna" -type "stringArray" 22 "NURBS Curves" "NURBS Surfaces" "Polygons" "Subdiv Surface" "Particles" "Particle Instance" "Fluids" "Strokes" "Image Planes" "UI" "Lights" "Cameras" "Locators" "Joints" "IK Handles" "Deformers" "Motion Trails" "Components" "Hair Systems" "Follicles" "Misc. UI" "Ornaments"  ;
	setAttr ".otfva" -type "Int32Array" 22 0 1 1 1 1 1
		 1 1 1 0 0 0 0 0 0 0 0 0
		 0 0 0 0 ;
	setAttr ".msaa" yes;
	setAttr ".fprt" yes;
select -ne :renderPartition;
	setAttr -s 6 ".st";
select -ne :renderGlobalsList1;
select -ne :defaultShaderList1;
	setAttr -s 9 ".s";
select -ne :postProcessList1;
	setAttr -s 2 ".p";
select -ne :defaultRenderUtilityList1;
	setAttr -s 3 ".u";
select -ne :defaultRenderingList1;
select -ne :initialShadingGroup;
	setAttr ".ro" yes;
select -ne :initialParticleSE;
	setAttr ".ro" yes;
select -ne :defaultRenderGlobals;
	addAttr -ci true -h true -sn "dss" -ln "defaultSurfaceShader" -dt "string";
	setAttr ".ren" -type "string" "arnold";
	setAttr ".dss" -type "string" "lambert1";
select -ne :defaultResolution;
	setAttr ".pa" 1;
select -ne :defaultColorMgtGlobals;
	setAttr ".cfe" yes;
	setAttr ".cfp" -type "string" "<MAYA_RESOURCES>/OCIO-configs/Maya2022-default/config.ocio";
	setAttr ".wsn" -type "string" "ACEScg";
select -ne :hardwareRenderGlobals;
	setAttr ".ctrs" 256;
	setAttr ".btrs" 512;
connectAttr "expression13.out[0]" "pCube_fpsr_sm.changed";
connectAttr "expression1.out[0]" "pCube_fpsr_sm.ty";
connectAttr "expression2.out[0]" "pCube_fpsr_sm.tz";
connectAttr "polyCube1.out" "pCube_fpsr_smShape.i";
connectAttr "expression15.out[0]" "pCube_fpsr_tm.changed";
connectAttr "expression16.out[0]" "pCube_fpsr_tm.ty";
connectAttr "expression17.out[0]" "pCube_fpsr_tm.tz";
connectAttr "polyCube2.out" "pCube_fpsr_tmShape.i";
connectAttr "expression14.out[0]" "pCube_fpsr_qs.changed";
connectAttr "expression3.out[0]" "pCube_fpsr_qs.tz";
connectAttr "expression4.out[0]" "pCube_fpsr_qs.ty";
connectAttr "shellDeformer1.rotationPivotPointsPP" "displayPoints1.inPointPositionsPP[0]"
		;
connectAttr "shellDeformer1.scalePivotPointsPP" "displayPoints1.inPointPositionsPP[1]"
		;
connectAttr "shellDeformer1.og[0]" "txt_fpsr_smShape.i";
connectAttr "shellDeformer3.og[0]" "txt_fpsr_tmShape.i";
connectAttr "shellDeformer2.og[0]" "txt_fpsr_qsShape.i";
relationship "link" ":lightLinker1" ":initialShadingGroup.message" ":defaultLightSet.message";
relationship "link" ":lightLinker1" ":initialParticleSE.message" ":defaultLightSet.message";
relationship "link" ":lightLinker1" "typeBlinnSG.message" ":defaultLightSet.message";
relationship "link" ":lightLinker1" "blinn1SG.message" ":defaultLightSet.message";
relationship "link" ":lightLinker1" "blinn_qsSG.message" ":defaultLightSet.message";
relationship "link" ":lightLinker1" "blinn_tmSG.message" ":defaultLightSet.message";
relationship "shadowLink" ":lightLinker1" ":initialShadingGroup.message" ":defaultLightSet.message";
relationship "shadowLink" ":lightLinker1" ":initialParticleSE.message" ":defaultLightSet.message";
relationship "shadowLink" ":lightLinker1" "typeBlinnSG.message" ":defaultLightSet.message";
relationship "shadowLink" ":lightLinker1" "blinn1SG.message" ":defaultLightSet.message";
relationship "shadowLink" ":lightLinker1" "blinn_qsSG.message" ":defaultLightSet.message";
relationship "shadowLink" ":lightLinker1" "blinn_tmSG.message" ":defaultLightSet.message";
connectAttr "layerManager.dli[0]" "defaultLayer.id";
connectAttr "renderLayerManager.rlmi[0]" "defaultRenderLayer.rlid";
connectAttr ":time1.o" "expression1.tim";
connectAttr "pCube_fpsr_sm.msg" "expression1.obm";
connectAttr ":time1.o" "expression2.tim";
connectAttr "pCube_fpsr_sm.msg" "expression2.obm";
connectAttr ":time1.o" "expression3.tim";
connectAttr "pCube_fpsr_qs.msg" "expression3.obm";
connectAttr ":time1.o" "expression4.tim";
connectAttr "pCube_fpsr_qs.msg" "expression4.obm";
connectAttr "txt_fpsr_sm.msg" "type1.transformMessage";
connectAttr "type1.vertsPerChar" "typeExtrude1.vertsPerChar";
connectAttr "groupid1.id" "typeExtrude1.cid";
connectAttr "groupid2.id" "typeExtrude1.bid";
connectAttr "groupid3.id" "typeExtrude1.eid";
connectAttr "type1.outputMesh" "typeExtrude1.in";
connectAttr "type1.extrudeMessage" "typeExtrude1.typeMessage";
connectAttr "groupId1.id" "typeExtrude1.charGroupId" -na;
connectAttr "groupId2.id" "typeExtrude1.charGroupId" -na;
connectAttr "groupId3.id" "typeExtrude1.charGroupId" -na;
connectAttr "groupId4.id" "typeExtrude1.charGroupId" -na;
connectAttr "groupId5.id" "typeExtrude1.charGroupId" -na;
connectAttr "groupId6.id" "typeExtrude1.charGroupId" -na;
connectAttr "groupId7.id" "typeExtrude1.charGroupId" -na;
connectAttr "typeBlinn.oc" "typeBlinnSG.ss";
connectAttr "txt_fpsr_smShape.iog" "typeBlinnSG.dsm" -na;
connectAttr "txt_fpsr_qsShape.iog" "typeBlinnSG.dsm" -na;
connectAttr "txt_fpsr_tmShape.iog" "typeBlinnSG.dsm" -na;
connectAttr "typeBlinnSG.msg" "materialInfo1.sg";
connectAttr "typeBlinn.msg" "materialInfo1.m";
connectAttr "typeExtrude1.out" "vectorAdjust1.ip[0].ig";
connectAttr "type1.grouping" "vectorAdjust1.grouping";
connectAttr "type1.manipulatorTransforms" "vectorAdjust1.manipulatorTransforms";
connectAttr "type1.alignmentMode" "vectorAdjust1.alignmentMode";
connectAttr "type1.vertsPerChar" "vectorAdjust1.vertsPerChar";
connectAttr "typeExtrude1.vertexGroupIds" "vectorAdjust1.vertexGroupIds";
connectAttr "vectorAdjust1.og[0]" "polySoftEdge1.ip";
connectAttr "txt_fpsr_smShape.wm" "polySoftEdge1.mp";
connectAttr "polySoftEdge1.out" "polyRemesh1.ip";
connectAttr "txt_fpsr_smShape.wm" "polyRemesh1.mp";
connectAttr "type1.remeshMessage" "polyRemesh1.typeMessage";
connectAttr "typeExtrude1.capComponents" "polyRemesh1.ics";
connectAttr "polyRemesh1.out" "polyAutoProj1.ip";
connectAttr "txt_fpsr_smShape.wm" "polyAutoProj1.mp";
connectAttr "polyAutoProj1.out" "shellDeformer1.ip[0].ig";
connectAttr "type1.vertsPerChar" "shellDeformer1.vertsPerChar";
connectAttr ":time1.o" "shellDeformer1.ti";
connectAttr "type1.grouping" "shellDeformer1.grouping";
connectAttr "type1.animationMessage" "shellDeformer1.typeMessage";
connectAttr "typeExtrude1.vertexGroupIds" "shellDeformer1.vertexGroupIds";
connectAttr "polyAutoProj2.out" "shellDeformer2.ip[0].ig";
connectAttr "type2.vertsPerChar" "shellDeformer2.vertsPerChar";
connectAttr ":time1.o" "shellDeformer2.ti";
connectAttr "type2.grouping" "shellDeformer2.grouping";
connectAttr "type2.animationMessage" "shellDeformer2.typeMessage";
connectAttr "typeExtrude2.vertexGroupIds" "shellDeformer2.vertexGroupIds";
connectAttr "polyRemesh2.out" "polyAutoProj2.ip";
connectAttr "txt_fpsr_qsShape.wm" "polyAutoProj2.mp";
connectAttr "polySoftEdge2.out" "polyRemesh2.ip";
connectAttr "txt_fpsr_qsShape.wm" "polyRemesh2.mp";
connectAttr "type2.remeshMessage" "polyRemesh2.typeMessage";
connectAttr "typeExtrude2.capComponents" "polyRemesh2.ics";
connectAttr "vectorAdjust2.og[0]" "polySoftEdge2.ip";
connectAttr "txt_fpsr_qsShape.wm" "polySoftEdge2.mp";
connectAttr "typeExtrude2.out" "vectorAdjust2.ip[0].ig";
connectAttr "type2.grouping" "vectorAdjust2.grouping";
connectAttr "type2.manipulatorTransforms" "vectorAdjust2.manipulatorTransforms";
connectAttr "type2.alignmentMode" "vectorAdjust2.alignmentMode";
connectAttr "type2.vertsPerChar" "vectorAdjust2.vertsPerChar";
connectAttr "typeExtrude2.vertexGroupIds" "vectorAdjust2.vertexGroupIds";
connectAttr "type2.vertsPerChar" "typeExtrude2.vertsPerChar";
connectAttr "groupid4.id" "typeExtrude2.cid";
connectAttr "groupid5.id" "typeExtrude2.bid";
connectAttr "groupid6.id" "typeExtrude2.eid";
connectAttr "type2.outputMesh" "typeExtrude2.in";
connectAttr "type2.extrudeMessage" "typeExtrude2.typeMessage";
connectAttr "groupId8.id" "typeExtrude2.charGroupId" -na;
connectAttr "groupId9.id" "typeExtrude2.charGroupId" -na;
connectAttr "groupId10.id" "typeExtrude2.charGroupId" -na;
connectAttr "groupId11.id" "typeExtrude2.charGroupId" -na;
connectAttr "groupId12.id" "typeExtrude2.charGroupId" -na;
connectAttr "groupId13.id" "typeExtrude2.charGroupId" -na;
connectAttr "groupId14.id" "typeExtrude2.charGroupId" -na;
connectAttr "txt_fpsr_qs.msg" "type2.transformMessage";
connectAttr "colorCondition1.oc" "blinn_sm.c";
connectAttr "blinn_sm.oc" "blinn1SG.ss";
connectAttr "pCube_fpsr_smShape.iog" "blinn1SG.dsm" -na;
connectAttr "blinn1SG.msg" "materialInfo2.sg";
connectAttr "blinn_sm.msg" "materialInfo2.m";
connectAttr "colorCondition1.msg" "materialInfo2.t" -na;
connectAttr "colorCondition2.oc" "blinn_qs.c";
connectAttr "blinn_qs.oc" "blinn_qsSG.ss";
connectAttr "pCube_fpsr_qsShape.iog" "blinn_qsSG.dsm" -na;
connectAttr "blinn_qsSG.msg" "materialInfo3.sg";
connectAttr "blinn_qs.msg" "materialInfo3.m";
connectAttr "colorCondition2.msg" "materialInfo3.t" -na;
connectAttr "pCube_fpsr_sm.ty" "expression13.in[0]";
connectAttr "pCube_fpsr_sm.tz" "expression13.in[1]";
connectAttr "pCube_fpsr_sm.msg" "expression13.obm";
connectAttr ":time1.o" "expression13.tim";
connectAttr "pCube_fpsr_sm.changed" "colorCondition1._cnd";
connectAttr "pCube_fpsr_qs.changed" "colorCondition2._cnd";
connectAttr ":time1.o" "expression14.tim";
connectAttr "pCube_fpsr_qs.ty" "expression14.in[0]";
connectAttr "pCube_fpsr_qs.tz" "expression14.in[1]";
connectAttr "pCube_fpsr_qs.msg" "expression14.obm";
connectAttr "pCube_fpsr_tm.ty" "expression15.in[0]";
connectAttr "pCube_fpsr_tm.tz" "expression15.in[1]";
connectAttr "pCube_fpsr_tm.msg" "expression15.obm";
connectAttr ":time1.o" "expression15.tim";
connectAttr ":time1.o" "expression16.tim";
connectAttr "pCube_fpsr_tm.msg" "expression16.obm";
connectAttr ":time1.o" "expression17.tim";
connectAttr "pCube_fpsr_tm.msg" "expression17.obm";
connectAttr "polyAutoProj3.out" "shellDeformer3.ip[0].ig";
connectAttr "type3.vertsPerChar" "shellDeformer3.vertsPerChar";
connectAttr ":time1.o" "shellDeformer3.ti";
connectAttr "type3.grouping" "shellDeformer3.grouping";
connectAttr "type3.animationMessage" "shellDeformer3.typeMessage";
connectAttr "typeExtrude3.vertexGroupIds" "shellDeformer3.vertexGroupIds";
connectAttr "polyRemesh3.out" "polyAutoProj3.ip";
connectAttr "txt_fpsr_tmShape.wm" "polyAutoProj3.mp";
connectAttr "polySoftEdge3.out" "polyRemesh3.ip";
connectAttr "txt_fpsr_tmShape.wm" "polyRemesh3.mp";
connectAttr "type3.remeshMessage" "polyRemesh3.typeMessage";
connectAttr "typeExtrude3.capComponents" "polyRemesh3.ics";
connectAttr "vectorAdjust3.og[0]" "polySoftEdge3.ip";
connectAttr "txt_fpsr_tmShape.wm" "polySoftEdge3.mp";
connectAttr "typeExtrude3.out" "vectorAdjust3.ip[0].ig";
connectAttr "type3.grouping" "vectorAdjust3.grouping";
connectAttr "type3.manipulatorTransforms" "vectorAdjust3.manipulatorTransforms";
connectAttr "type3.alignmentMode" "vectorAdjust3.alignmentMode";
connectAttr "type3.vertsPerChar" "vectorAdjust3.vertsPerChar";
connectAttr "typeExtrude3.vertexGroupIds" "vectorAdjust3.vertexGroupIds";
connectAttr "type3.vertsPerChar" "typeExtrude3.vertsPerChar";
connectAttr "groupid7.id" "typeExtrude3.cid";
connectAttr "groupid8.id" "typeExtrude3.bid";
connectAttr "groupid9.id" "typeExtrude3.eid";
connectAttr "type3.outputMesh" "typeExtrude3.in";
connectAttr "type3.extrudeMessage" "typeExtrude3.typeMessage";
connectAttr "groupId15.id" "typeExtrude3.charGroupId" -na;
connectAttr "groupId16.id" "typeExtrude3.charGroupId" -na;
connectAttr "groupId17.id" "typeExtrude3.charGroupId" -na;
connectAttr "groupId18.id" "typeExtrude3.charGroupId" -na;
connectAttr "groupId19.id" "typeExtrude3.charGroupId" -na;
connectAttr "groupId20.id" "typeExtrude3.charGroupId" -na;
connectAttr "groupId21.id" "typeExtrude3.charGroupId" -na;
connectAttr "txt_fpsr_tm.msg" "type3.transformMessage";
connectAttr "pCube_fpsr_tm.changed" "colorCondition3._cnd";
connectAttr "colorCondition3.oc" "blinn_tm.c";
connectAttr "blinn_tm.oc" "blinn_tmSG.ss";
connectAttr "pCube_fpsr_tmShape.iog" "blinn_tmSG.dsm" -na;
connectAttr "blinn_tmSG.msg" "materialInfo4.sg";
connectAttr "blinn_tm.msg" "materialInfo4.m";
connectAttr "colorCondition3.msg" "materialInfo4.t" -na;
connectAttr "expression14.msg" "MayaNodeEditorSavedTabsInfo.tgi[0].ni[0].dn";
connectAttr "pCube_fpsr_qs.msg" "MayaNodeEditorSavedTabsInfo.tgi[0].ni[1].dn";
connectAttr "expression3.msg" "MayaNodeEditorSavedTabsInfo.tgi[0].ni[2].dn";
connectAttr "colorCondition2.msg" "MayaNodeEditorSavedTabsInfo.tgi[0].ni[3].dn";
connectAttr "pCube_fpsr_tmShape.msg" "MayaNodeEditorSavedTabsInfo.tgi[0].ni[4].dn"
		;
connectAttr "expression17.msg" "MayaNodeEditorSavedTabsInfo.tgi[0].ni[5].dn";
connectAttr "blinn_qs.msg" "MayaNodeEditorSavedTabsInfo.tgi[0].ni[6].dn";
connectAttr "blinn_tmSG.msg" "MayaNodeEditorSavedTabsInfo.tgi[0].ni[7].dn";
connectAttr "pCube_fpsr_qsShape.msg" "MayaNodeEditorSavedTabsInfo.tgi[0].ni[8].dn"
		;
connectAttr "blinn_tm.msg" "MayaNodeEditorSavedTabsInfo.tgi[0].ni[9].dn";
connectAttr "expression15.msg" "MayaNodeEditorSavedTabsInfo.tgi[0].ni[10].dn";
connectAttr "expression4.msg" "MayaNodeEditorSavedTabsInfo.tgi[0].ni[11].dn";
connectAttr "polyCube2.msg" "MayaNodeEditorSavedTabsInfo.tgi[0].ni[12].dn";
connectAttr "pCube_fpsr_tm.msg" "MayaNodeEditorSavedTabsInfo.tgi[0].ni[13].dn";
connectAttr ":time1.msg" "MayaNodeEditorSavedTabsInfo.tgi[0].ni[14].dn";
connectAttr "expression16.msg" "MayaNodeEditorSavedTabsInfo.tgi[0].ni[15].dn";
connectAttr "blinn_qsSG.msg" "MayaNodeEditorSavedTabsInfo.tgi[0].ni[16].dn";
connectAttr "colorCondition3.msg" "MayaNodeEditorSavedTabsInfo.tgi[0].ni[17].dn"
		;
connectAttr "blinn_tmSG.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[0].dn"
		;
connectAttr "colorCondition1.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[1].dn"
		;
connectAttr "colorCondition3.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[2].dn"
		;
connectAttr "blinn1SG.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[3].dn"
		;
connectAttr "blinn_sm.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[4].dn"
		;
connectAttr "blinn_tm.msg" "hyperShadePrimaryNodeEditorSavedTabsInfo.tgi[0].ni[5].dn"
		;
connectAttr "typeBlinnSG.pa" ":renderPartition.st" -na;
connectAttr "blinn1SG.pa" ":renderPartition.st" -na;
connectAttr "blinn_qsSG.pa" ":renderPartition.st" -na;
connectAttr "blinn_tmSG.pa" ":renderPartition.st" -na;
connectAttr "typeBlinn.msg" ":defaultShaderList1.s" -na;
connectAttr "blinn_sm.msg" ":defaultShaderList1.s" -na;
connectAttr "blinn_qs.msg" ":defaultShaderList1.s" -na;
connectAttr "blinn_tm.msg" ":defaultShaderList1.s" -na;
connectAttr "colorCondition1.msg" ":defaultRenderUtilityList1.u" -na;
connectAttr "colorCondition2.msg" ":defaultRenderUtilityList1.u" -na;
connectAttr "colorCondition3.msg" ":defaultRenderUtilityList1.u" -na;
connectAttr "defaultRenderLayer.msg" ":defaultRenderingList1.r" -na;
// End of fpsr_algorithms.ma
