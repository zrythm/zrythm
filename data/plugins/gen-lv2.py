# SPDX-FileCopyrightText: © 2022, 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense
#!/usr/bin/env python3
# This script generates the given LV2 plugin using Faust.

import os
import re
import sys
import shutil
import subprocess

def remove_dynamic_manifest_code(cpp_file):
    with open(cpp_file, 'r', encoding='utf-8') as f:
        content = f.read()

    # Remove the dynamic manifest functions
    patterns_to_remove = [
        r'extern "C"\s*LV2_SYMBOL_EXPORT\s*int\s*lv2_dyn_manifest_open.*?^}$',
        r'extern "C"\s*LV2_SYMBOL_EXPORT\s*int\s*lv2_dyn_manifest_get_subjects.*?^}$',
        r'extern "C"\s*LV2_SYMBOL_EXPORT\s*int\s*lv2_dyn_manifest_get_data.*?^}$',
        r'extern "C"\s*LV2_SYMBOL_EXPORT\s*void\s*lv2_dyn_manifest_close.*?^}$',
        r'int\s+main\s*\(\s*\).*?^}$',
        r'static string mangle\(const string &s\).*?^}$',
        r'static unsigned steps\(float min, float max, float step\).*?^}$',
        r'static bool is_xmlstring\(const char \*s\).*?^}$'
    ]

    for pattern in patterns_to_remove:
        content = re.sub(pattern, '', content, flags=re.DOTALL | re.MULTILINE)

    with open(cpp_file, 'w', encoding='utf-8') as f:
        f.write(content)

def main():
    faust2lv2 = sys.argv[1]
    dsp_file = sys.argv[2]
    dsp_filename = os.path.basename(dsp_file)
    pl_underscored_name = os.path.splitext(dsp_filename)[0]
    uri_prefix = sys.argv[3]
    # pl_uri = uri_prefix + pl_underscored_name
    prv_dir = os.path.abspath(sys.argv[4])
    out_filename = sys.argv[5]
    pl_type = sys.argv[6]
    utils_lib = sys.argv[7]
    utils_lib_filename = os.path.basename(utils_lib)
    generated_src_output = sys.argv[8]

    os.makedirs(prv_dir, exist_ok=True)

    shutil.copy(dsp_file, os.path.join(prv_dir, dsp_filename))
    shutil.copy(utils_lib, os.path.join(prv_dir, utils_lib_filename))

    os.chdir(prv_dir)

    faust_args = ["-keep", "-vec"]
    if pl_type == "InstrumentPlugin":
        faust_args.extend(["-novoicectrls", "-nvoices", "16"])

    subprocess.run([faust2lv2, "-uri-prefix", uri_prefix] + faust_args + [dsp_filename], check=True)

    if pl_type == "InstrumentPlugin":
        print("done")
    else:
        ttl_file = f"{pl_underscored_name}.lv2/{pl_underscored_name}.ttl"
        with open(ttl_file, 'r', encoding='utf-8') as f:
            content = f.read()
        content = content.replace("a lv2:Plugin", f"a lv2:{pl_type}, lv2:Plugin")
        with open(ttl_file, 'w', encoding='utf-8') as f:
            f.write(content)

    os.chdir('..')

    dest_lv2_dir = os.path.join(generated_src_output, f"{pl_underscored_name}.lv2")
    os.makedirs(dest_lv2_dir, exist_ok=True)
    for file in os.listdir(f"{prv_dir}/{pl_underscored_name}.lv2"):
        if file.endswith('.ttl'):
            dest_file = os.path.join(dest_lv2_dir, file + '.in')
            shutil.copy(f"{prv_dir}/{pl_underscored_name}.lv2/{file}", dest_file)

    for file in [f"{pl_underscored_name}.ttl.in", "manifest.ttl.in"]:
        file_path = os.path.join(dest_lv2_dir, file)
        with open(file_path, 'r', encoding='utf-8') as f:
            content = f.read()
        content = content.replace(f"<{pl_underscored_name}.so>",
                                  f"<{pl_underscored_name}@CMAKE_SHARED_LIBRARY_SUFFIX@>")
        with open(file_path, 'w', encoding='utf-8') as f:
            f.write(content)

    shutil.copy(f"{prv_dir}/{pl_underscored_name}/{pl_underscored_name}.cpp", dest_lv2_dir)
    cpp_file = os.path.join(dest_lv2_dir, f"{pl_underscored_name}.cpp")
    remove_dynamic_manifest_code(cpp_file)
    with open(cpp_file, 'r', encoding='utf-8') as f:
        content = f.read()
    content = content.replace('"https://faustlv2.bitbucket.io"', f'"{uri_prefix}"')
    with open(cpp_file, 'w', encoding='utf-8') as f:
        f.write(content)

    shutil.rmtree(out_filename, ignore_errors=True)
    shutil.move(f"{prv_dir}/{pl_underscored_name}.lv2", out_filename)

    os.remove(os.path.join(prv_dir, dsp_filename))
    os.remove(os.path.join(prv_dir, utils_lib_filename))
    shutil.rmtree(os.path.join(prv_dir, pl_underscored_name), ignore_errors=True)
    for item in os.listdir(prv_dir):
        if item.endswith('.lv2'):
            shutil.rmtree(os.path.join(prv_dir, item))
    os.rmdir(prv_dir)

if __name__ == "__main__":
    main()
