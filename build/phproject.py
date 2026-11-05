#!/usr/bin/env python3
"""Primal Hunt's project files, derived once from the release's.

  build/phproject.py <release.dsp> <out.dsp> <output dir> [<configuration>]

Primal Hunt's game DLLs are AvP2 1.0.9.6's code carried on by Third Law,
built with VC6 SP5 (progress/COMPILERS.md). Their projects here are copies
of the release's: the project renamed <name>_PH (so that one make holds
both games' rules), its "Final" configuration defining PRIMAL_HUNT (the
macro Primal Hunt's changes go behind: docs/PLAN.md, ground rules) and
putting its objects and output in <output dir>. A project with no "Final"
(the dedicated server, ServerApp.dsp) is given its "Release" instead, the
configuration tools/dsp2mk.py builds of it. The other configurations
are left as they are, unused. Run once to start a project; from then on the
_PH project is a source file of its own (Third Law's new files go into it
alone).
"""
import re
import sys


def main():
    src, out, outdir = sys.argv[1:4]
    config = sys.argv[4] if len(sys.argv) > 4 else "Final"
    # the define PRIMAL_HUNT goes after: Final's _FINAL, Release's NDEBUG
    anchor = {"Final": "_FINAL", "Release": "NDEBUG"}[config]
    text = open(src, encoding="latin-1").read()
    name = re.search(r'Project File - Name="([^"]+)"', text).group(1)
    ph = name + "_PH"
    text = text.replace(f'Name="{name}"', f'Name="{ph}"')
    text = text.replace(f'"{name} - Win32 ', f'"{ph} - Win32 ').replace(f"={name} - Win32 ", f"={ph} - Win32 ")
    head, sep, rest = text.partition(f'"$(CFG)" == "{ph} - Win32 {config}"')
    if not sep:
        sys.exit(f"phproject: {src}: no {config} configuration")
    final, sep2, tail = rest.partition("\n!E")
    final = re.sub(r'(# PROP (?:Output|Intermediate)_Dir) "[^"]*"', rf'\1 "{outdir}"', final)
    final = re.sub(r'(/out:)"[^"\\/]*([\\/][^"]*)"', rf'\1"{outdir}\2"', final)
    final = re.sub(r'(/pdb:)"[^"\\/]*([\\/][^"]*)"', rf'\1"{outdir}\2"', final)
    final = re.sub(rf'(# ADD CPP .*?)( /D "{anchor}")', r'\1\2 /D "PRIMAL_HUNT"', final)
    if final.count('/D "PRIMAL_HUNT"') != 1:
        sys.exit(f"phproject: {src}: the {config} configuration's ADD CPP has no /D {anchor}")
    open(out, "w", encoding="latin-1").write(head + sep + final + sep2 + tail)


if __name__ == "__main__":
    main()
