#!/usr/bin/env python3
# SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense
#
# This script generates JUCE plugin source code from Faust DSP files.

import argparse
import os
import re
import shutil
import subprocess
import sys


def strip_trailing_whitespace(cpp_file: str) -> None:
    """Remove trailing whitespace from each line in the file."""
    with open(cpp_file, 'r', encoding='utf-8') as f:
        lines = f.readlines()

    with open(cpp_file, 'w', encoding='utf-8') as f:
        for line in lines:
            f.write(line.rstrip() + '\n')


def remove_unused_code(cpp_file: str) -> None:
    """Remove unused code from generated Faust plugin source."""
    with open(cpp_file, 'r', encoding='utf-8') as f:
        content = f.read()

    replacement = '/* (removed unused code - see gen-juce.py) */'
    sections = [
        'SoundUI.h',
        'JuceOSCUI.h',
        'JuceGUI.h',
    ]

    for section in sections:
        pattern = rf'/\*+\s*BEGIN\s+{re.escape(section)}.*?/\*+\s*END\s+{re.escape(section)}.*?\*+/'
        content = re.sub(pattern, replacement, content, flags=re.DOTALL)

    with open(cpp_file, 'w', encoding='utf-8') as f:
        f.write(content)


def remove_gui_code(cpp_file: str) -> None:
    """
    Remove GUI-related code from the generated plugin.

    This strips out the editor class and related methods so the plugin
    uses the host's generic UI instead.
    """
    with open(cpp_file, 'r', encoding='utf-8') as f:
        content = f.read()

    # Remove GUI includes
    content = re.sub(r'#include <juce_gui_basics/juce_gui_basics\.h>\n?', '', content)
    content = re.sub(r'#include <juce_gui_extra/juce_gui_extra\.h>\n?', '', content)

    # Remove the entire FaustPlugInAudioProcessorEditor class definition
    # Match from class declaration to closing brace + semicolon
    content = re.sub(
        r'class FaustPlugInAudioProcessorEditor : public juce::AudioProcessorEditor.*?\n};\n',
        '',
        content,
        flags=re.DOTALL
    )

    # Remove the editor constructor implementation
    content = re.sub(
        r'FaustPlugInAudioProcessorEditor::FaustPlugInAudioProcessorEditor.*?^\}\n',
        '',
        content,
        flags=re.DOTALL | re.MULTILINE
    )

    # Remove the editor paint implementation
    content = re.sub(
        r'void FaustPlugInAudioProcessorEditor::paint.*?^\}\n',
        '',
        content,
        flags=re.DOTALL | re.MULTILINE
    )

    # Remove the editor resized implementation
    content = re.sub(
        r'void FaustPlugInAudioProcessorEditor::resized.*?^\}\n',
        '',
        content,
        flags=re.DOTALL | re.MULTILINE
    )

    # Make hasEditor return false
    content = re.sub(
        r'bool FaustPlugInAudioProcessor::hasEditor\(\) const.*?\{[^}]*\}',
        'bool FaustPlugInAudioProcessor::hasEditor() const { return false; }',
        content,
        flags=re.DOTALL
    )

    # Make createEditor return nullptr (keep the function, just change the body)
    content = re.sub(
        r'juce::AudioProcessorEditor\* FaustPlugInAudioProcessor::createEditor\(\).*?\{[^}]*return new [^;]+;\s*\}',
        'juce::AudioProcessorEditor* FaustPlugInAudioProcessor::createEditor() { return nullptr; }',
        content,
        flags=re.DOTALL
    )

    with open(cpp_file, 'w', encoding='utf-8') as f:
        f.write(content)


def fix_juce_includes(cpp_file: str, include_gui: bool = True) -> None:
    """
    Replace the Projucer-specific include with direct JUCE module includes.

    The generated code includes #include "JuceLibraryCode/JuceHeader.h"
    or #include "../JuceLibraryCode/JuceHeader.h" which doesn't exist when
    building with CMake. We replace it with direct module includes.
    """
    with open(cpp_file, 'r', encoding='utf-8') as f:
        content = f.read()

    # Replace the JuceHeader include with direct module includes
    # These are the modules needed by faust2juce generated code
    base_includes = '''#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_dsp/juce_dsp.h>'''

    if include_gui:
        base_includes += '''
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>'''

    # Handle both with and without ../ prefix
    content = content.replace(
        '#include "JuceLibraryCode/JuceHeader.h"',
        base_includes
    )
    content = content.replace(
        '#include "../JuceLibraryCode/JuceHeader.h"',
        base_includes
    )

    with open(cpp_file, 'w', encoding='utf-8') as f:
        f.write(content)


def main():
    parser = argparse.ArgumentParser(
        description='Generate JUCE plugin source from Faust DSP'
    )
    parser.add_argument(
        'dsp_file',
        help='Input .dsp file (already configured from .dsp.in)'
    )
    parser.add_argument(
        '-o', '--output-dir',
        required=True,
        help='Output directory for generated source'
    )
    parser.add_argument(
        '-j', '--juce-modules-dir',
        required=True,
        help='Path to JUCE modules directory'
    )
    parser.add_argument(
        '-u', '--utils-lib',
        required=True,
        help='Path to zrythm-utils.lib'
    )
    parser.add_argument(
        '-m', '--midi',
        action='store_true',
        help='Enable MIDI support with JUCE Synthesizer polyphony'
    )
    parser.add_argument(
        '--no-gui',
        action='store_true',
        help='Disable GUI generation (use host-provided UI)'
    )
    parser.add_argument(
        '--faust2juce',
        default='faust2juce',
        help='Path to faust2juce executable (default: faust2juce)'
    )

    args = parser.parse_args()

    dsp_file = os.path.abspath(args.dsp_file)
    output_dir = os.path.abspath(args.output_dir)
    juce_modules_dir = os.path.abspath(args.juce_modules_dir)
    utils_lib = os.path.abspath(args.utils_lib)

    if not os.path.isfile(dsp_file):
        print(f"Error: DSP file not found: {dsp_file}", file=sys.stderr)
        sys.exit(1)

    if not os.path.isfile(utils_lib):
        print(f"Error: Utils library not found: {utils_lib}", file=sys.stderr)
        sys.exit(1)

    dsp_filename = os.path.basename(dsp_file)
    plugin_name = os.path.splitext(dsp_filename)[0]
    utils_lib_filename = os.path.basename(utils_lib)

    # Create a temporary working directory (unique per plugin to avoid race conditions)
    work_dir = os.path.join(output_dir, f'.gen-tmp-{plugin_name}')
    os.makedirs(work_dir, exist_ok=True)

    # Copy DSP file and utils library to work directory
    work_dsp = os.path.join(work_dir, dsp_filename)
    work_utils = os.path.join(work_dir, utils_lib_filename)
    shutil.copy(dsp_file, work_dsp)
    shutil.copy(utils_lib, work_utils)

    # Build faust2juce arguments
    faust_args = [
        args.faust2juce,
        '-jucemodulesdir', juce_modules_dir,
    ]

    if args.midi:
        faust_args.extend(['-midi'])

    faust_args.append(dsp_filename)

    # Run faust2juce
    original_dir = os.getcwd()
    os.chdir(work_dir)

    try:
        print(f"Running: {' '.join(faust_args)}")
        subprocess.run(faust_args, check=True)
    except subprocess.CalledProcessError as e:
        print(f"Error: faust2juce failed with code {e.returncode}", file=sys.stderr)
        sys.exit(1)
    finally:
        os.chdir(original_dir)

    # Find the generated output directory (faust2juce creates a folder with the plugin name)
    generated_dir = os.path.join(work_dir, plugin_name)
    generated_cpp = os.path.join(generated_dir, 'FaustPluginProcessor.cpp')

    if not os.path.isfile(generated_cpp):
        print(f"Error: Generated source not found: {generated_cpp}", file=sys.stderr)
        sys.exit(1)

    # Create output directory
    final_output_dir = os.path.join(output_dir, plugin_name)
    os.makedirs(final_output_dir, exist_ok=True)

    # Fix JUCE includes (exclude GUI includes if --no-gui)
    fix_juce_includes(generated_cpp, include_gui=not args.no_gui)

    # Remove unused code
    remove_unused_code(generated_cpp)

    # Remove GUI code if requested
    if args.no_gui:
        remove_gui_code(generated_cpp)

    # Strip trailing whitespace
    strip_trailing_whitespace(generated_cpp)

    # Copy generated source to output directory
    final_cpp = os.path.join(final_output_dir, 'FaustPluginProcessor.cpp')
    shutil.copy(generated_cpp, final_cpp)

    # Clean up work directory
    shutil.rmtree(work_dir, ignore_errors=True)

    print(f"Generated: {final_cpp}")


if __name__ == "__main__":
    main()
