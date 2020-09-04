#!/usr/bin/env python3
import sys
import re
import json
from typing import NamedTuple, Dict, Any, Optional, List, Tuple
from collections import defaultdict
import difflib

class call_info_t(NamedTuple):
    function_name: str
    thread: int
    frame: int
    info: Dict[str, Any]


def set_path_value(d: Dict[str, Any], p: List[str], v: Any):
    for key in p[:-1]:
        if key not in d:
            d[key] = dict()
        d = d[key]
    d[p[-1]] = v

def get_indent(s: str):
    return len(s) - len(s.lstrip(" "))

def parse_dump(dumpfile_path: str) -> List[call_info_t]:
    start_re = re.compile("Thread (\d+), Frame (\d+):")
    current_entry: Optional[call_info_t] = None
    current_info_path: List[str] = []
    current_indent = 0
    entries: List[call_info_t] = list()
    for line in open(dumpfile_path):
        line = line[:-1] # strip newline
        if not line.strip():
            continue
        m = start_re.match(line)
        if m:
            if current_entry is not None:
                entries.append(current_entry)
                current_entry = None
            thread = int(m.group(1))
            frame = int(m.group(2))
        else:
            indentation_level = get_indent(line) // 4
            if indentation_level == 0:
                function_name = line.split("(")[0]
                #print(function_name)
                current_entry = call_info_t(function_name, thread, frame, dict())
            elif current_entry is not None:
                key, value = (s.strip() for s in line.split(":")[:2])
                info_depth = indentation_level - 1
                current_info_path = current_info_path[:info_depth]
                current_info_path.append(key)
                set_path_value(current_entry.info, current_info_path + ["value"], value)
    if current_entry:
        entries.append(current_entry)
    return entries


def call_counts(entries:List[call_info_t]):
    counts: Dict[str, int] = defaultdict(lambda:0)
    for entry in entries:
        counts[entry.function_name] += 1
    return counts

def best_match(ratios: List[List[float]]) -> Optional[Tuple[int, int, float]]:
    current: Optional[Tuple[int, int, float]] = None
    for i, row in enumerate(ratios):
        for j, value in enumerate(row):
            if value >= 0:
                if current is None or ratios[current[0]][current[1]] < value:
                    current = (i, j, value)
    return current


def best_matches(ratios: List[List[float]]) -> List[Tuple[int, int, float]]:
    result: List[Tuple[int, int, float]] = list()
    while True:
        match = best_match(ratios)
        if match is not None:
            result.append(match)
            for i in range(len(ratios[0])):
                ratios[match[0]][i] = -1
            for i in range(len(ratios)):
                ratios[i][match[1]] = -1
        else:
            break
    return result


pointer_re = re.compile("0x[0-9A-Fa-f]+")
def get_diffable_function_infos(calls: List[call_info_t], function_name: str) -> List[str]:
    return list([
        re.sub(pointer_re, "0xffff", json.dumps(entry, indent = 2))
        for entry in filter(lambda entry:entry.function_name == function_name, calls)
    ])


def make_match_matrix(a: List[str], b: List[str]) -> List[List[float]]:
    result: List[List[float]] = list()
    return list([
        list([
            difflib.SequenceMatcher(None, ai, bj).ratio()
            for bj in b
        ])
        for ai in a
    ])


core_entries = parse_dump(sys.argv[1])
vkrunner_entries = parse_dump(sys.argv[2])

core_counts = call_counts(core_entries)
vkrunner_counts = call_counts(vkrunner_entries)
all_function_names = set(core_counts.keys()).union(set(vkrunner_counts.keys()))
for function_name in all_function_names:
    if function_name.startswith("vkGet"):
        continue
    core_infos = get_diffable_function_infos(core_entries, function_name)
    vkrunner_infos = get_diffable_function_infos(vkrunner_entries, function_name)
    while len(core_infos) < len(vkrunner_infos):
        core_infos.append("")
    while len(core_infos) > len(vkrunner_infos):
        vkrunner_infos.append("")
    match_matrix = make_match_matrix(core_infos, vkrunner_infos)
    for match in best_matches(match_matrix):
        a = core_infos[match[0]]
        b = vkrunner_infos[match[1]]
        ratio = match[2]
        if ratio < 1:
            print("\n\n>> %s <<\n" % function_name)
            diff = difflib.unified_diff(
                a.splitlines(keepends=True),
                b.splitlines(keepends=True)
            )
            print("".join(diff), end="")
