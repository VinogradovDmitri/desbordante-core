#!/usr/bin/env python3
"""Prepare a review brief and granular phase todos.

This command does not review code and never creates commits or patches. It
records the selected delivery contract so the LLM can execute it consistently.
"""

from __future__ import annotations

import argparse
import math
import os
import re
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
OUTPUT_MODES = {"commits", "patches", "report"}
REVIEW_MODES = {"immersion", "design", "performance"}
VERIFY_OPTIONS = {"build", "tests", "python", "snapshots", "determinism", "profiling"}

# Shorthand for accepted values (the long form is always recorded in the brief).
MODE_ALIAS = {"c": "commits", "p": "patches", "r": "report"}
REVIEW_ALIAS = {"i": "immersion", "d": "design", "p": "performance"}
VERIFY_ALIAS = {"b": "build", "t": "tests", "py": "python", "s": "snapshots", "det": "determinism", "prof": "profiling"}


def git(*args: str, check: bool = True) -> str:
    result = subprocess.run(
        ["git", *args],
        cwd=ROOT,
        check=check,
        capture_output=True,
        text=True,
    )
    if check:
        return result.stdout.strip()
    return result.stdout.strip()


def resolve_revision(value: str | None, label: str) -> str:
    if value:
        try:
            return git("rev-parse", "--verify", value)
        except subprocess.CalledProcessError as error:
            raise SystemExit(f"Cannot resolve {label} revision {value!r}.") from error

    if label == "base":
        upstream = git("rev-parse", "--abbrev-ref", "--symbolic-full-name", "@{upstream}", check=False)
        if not upstream:
            raise SystemExit(
                "No configured upstream for the current branch. "
                "Run `make review BASE=<revision>` or configure a tracking branch."
            )
        value = upstream
    else:
        value = "HEAD"

    try:
        return git("rev-parse", "--verify", value)
    except subprocess.CalledProcessError as error:
        raise SystemExit(f"Cannot resolve {label} revision {value!r}.") from error


def choose_mode(value: str | None) -> str:
    value = value or os.environ.get("MODE") or ""
    if not value and sys.stdin.isatty():
        value = input("Review output [commits(c)/patches(p)/report(r)] (report): ").strip()
    value = value or "report"
    value = MODE_ALIAS.get(value.lower(), value.lower())
    if value not in OUTPUT_MODES:
        raise SystemExit(f"Invalid review output {value!r}; choose commits, patches, or report (c/p/r).")
    return value


def parse_positive_float(value: str | None, label: str, default: float) -> float:
    value = value or default
    try:
        parsed = float(value)
    except (TypeError, ValueError) as error:
        raise SystemExit(f"{label} must be a positive number.") from error
    if parsed <= 0:
        raise SystemExit(f"{label} must be a positive number.")
    return parsed


def resolve_hours(value: str | None) -> float:
    value = value or os.environ.get("HOURS") or ""
    if not value and sys.stdin.isatty():
        value = input("Estimated review hours (3): ").strip()
    return parse_positive_float(value or None, "HOURS", 3.0)


def parse_list(value: str, allowed: set[str], short: dict[str, str], label: str) -> list[str]:
    chosen: list[str] = []
    invalid: list[str] = []
    for raw in value.split(","):
        token = raw.strip().lower()
        if not token:
            continue
        resolved = short.get(token, token)
        if resolved not in allowed:
            invalid.append(token)
        else:
            chosen.append(resolved)
    chosen = list(dict.fromkeys(chosen))
    if invalid or not chosen:
        allowed_text = ", ".join(sorted(allowed))
        raise SystemExit(f"Invalid {label} {invalid or value!r}; use: {allowed_text}.")
    return chosen


def resolve_review_modes(value: str | None) -> list[str]:
    value = value or os.environ.get("REVIEW_MODES") or ""
    if not value and sys.stdin.isatty():
        value = input("Review modes, comma-separated [immersion(i)/design(d)/performance(p)] (all): ").strip()
    return parse_list(value or "immersion,design,performance", REVIEW_MODES, REVIEW_ALIAS, "REVIEW_MODES")


def resolve_verify(value: str | None) -> list[str]:
    value = value or os.environ.get("VERIFY") or ""
    if not value and sys.stdin.isatty():
        value = input("Verification, comma-separated [build(b)/tests(t)/python(py)/snapshots(s)/determinism(det)/profiling(prof)] (build): ").strip()
    return parse_list(value or "build", VERIFY_OPTIONS, VERIFY_ALIAS, "VERIFY")


def phase_count(mode: str, review_modes: list[str], hours: float, requested: str | None) -> int:
    requested_count = 0
    if requested:
        try:
            requested_count = int(requested)
        except ValueError as error:
            raise SystemExit("PHASES must be a positive integer.") from error
        if requested_count <= 0:
            raise SystemExit("PHASES must be a positive integer.")

    # Scope + selected review modes + verification + delivery/report + second pass.
    minimum = 1 + len(review_modes) + (3 if mode != "report" else 2)
    return max(math.ceil(hours / 2), requested_count, minimum)


def phase_roles(mode: str, review_modes: list[str], count: int) -> list[str]:
    roles = ["scope-and-interview"] + [f"{item}-review" for item in review_modes]
    tail = ["verification", "delivery", "report-and-second-pass"]
    if mode == "report":
        tail = ["verification", "report-and-second-pass"]

    extra = count - len(roles) - len(tail)
    roles.extend(f"review-slice-{index:02d}" for index in range(1, extra + 1))
    return roles + tail


def phase_checklist(role: str, mode: str, verify: list[str]) -> tuple[str, list[str]]:
    common = [
        "§CLAUDE §10 S1 — keep exactly one active phase and one file per planned phase.",
        "§CLAUDE §10 S2 — use an explicit governing checklist for this phase.",
        "§CLAUDE §10 S3 — mark in_progress before work and completed immediately after verification.",
        "§CLAUDE §10 S4 — walk the phase checklist top to bottom.",
        "§CLAUDE §10 S5 — completed means the command/result is recorded in the session log.",
        "§DEVELOPMENT §6 — run the applicable validation chain before phase exit.",
    ]
    verify_items = {
        "build": "Run the selected build from llm/DEVELOPMENT.md §6 (make -j<N>).",
        "tests": "Run the targeted pattern tests (ctest --test-dir build -R \"<algo>\").",
        "python": "Run the selected Python tests.",
        "snapshots": "Run the example snapshot checks.",
        "determinism": "Run the determinism probe.",
        "profiling": "Run the selected profiling/benchmark checks (PREWORK B/C gate).",
    }
    checklists: dict[str, list[str]] = {
        "scope-and-interview": [
            "Resolve the base and head revisions and confirm the target is not the working tree.",
            "Record selected output mode, review modes, estimated hours, and verification scope.",
            "Read the full target diff and enumerate the exact footprint before findings.",
            "Record known issues, exclusions, and stop conditions in the session brief.",
        ],
        "immersion-review": [
            "Compare the implementation against the supplied task/paper requirements.",
            "Record a verdict for every footprint area and distinguish missing text from missing behavior.",
            "Mark absent algorithm text as Pass with an explicit report note, never invent requirements.",
        ],
        "design-review": [
            "Walk llm/RULES.md sections 1–19 in order and record a verdict for every applicable box.",
            "Read the relevant tests, bindings, examples, and comparable implementations before findings.",
            "Attach file:line evidence and a concrete fix to every Fail verdict.",
        ],
        "performance-review": [
            "Confirm the performance interview selected datasets and tools before profiling.",
            "Check PREWORK B/C only when performance or benchmarking applies.",
            "Inspect hot loops, allocations, containers, parallelism, and determinism without guessing.",
        ],
        "verification": [verify_items[item] for item in verify] + [
            "If a check fails, fix it and restart verification from the first failed step.",
        ],
        "delivery": [
            "commits mode: implement one accepted finding per focused commit in the worktree.",
            "patches mode: generate one stable numbered patch per accepted finding.",
            "Map every delivery artifact to a report entry and verify the resulting series.",
        ],
        "report-and-second-pass": [
            "Write report.md with severity-ordered findings, evidence, fixes, and not-checked items.",
            "report mode: put every suggested fix in a fenced code block and make no code changes.",
            "Run the second pass for false positives, severity, coverage, and verification gaps.",
            "Delete only fully verified phase files and leave incomplete work visible.",
        ],
    }
    if role.startswith("review-slice-"):
        checklists[role] = [
            "Assign an exact file/commit boundary to this slice before reading it.",
            "Review every assigned item and record findings with file:line evidence.",
            "Record the slice result and handoff risks in the session brief.",
        ]
    title = role.replace("-", " ").title()
    if role == "delivery":
        title = f"Delivery ({mode})"
    return title, common + checklists[role]


def source_checklist(role: str) -> tuple[str, list[str]]:
    source = {"design-review": ("RULES.md", ROOT / "llm/RULES.md", 19),
              "performance-review": ("PERFORMANCE.md", ROOT / "llm/PERFORMANCE.md", 13)}.get(role)
    if source is None:
        return "", []

    label, path, last_section = source
    items: list[str] = []
    section_number = 0
    section_title = ""
    current: list[str] | None = None

    def flush() -> None:
        if current is not None:
            text = " ".join(current[1:])
            items.append(f"§{label} §{current[0]} — {section_title}: {text}")

    for raw_line in path.read_text(encoding="utf-8").splitlines():
        heading = re.match(r"^##\s+(\d+)\.\s+(.*)$", raw_line)
        if heading:
            flush()
            section_number = int(heading.group(1))
            section_title = heading.group(2).strip()
            current = None
            continue
        if not 1 <= section_number <= last_section:
            continue
        item = re.match(r"^- \[ \]\s+(.*)$", raw_line)
        if item:
            flush()
            current = [str(section_number), item.group(1).strip()]
        elif current is not None and raw_line.startswith("      "):
            current.append(raw_line.strip())
    flush()
    return label, items


def write_phase(path: Path, number: int, total: int, role: str, mode: str, hours: float, verify: list[str]) -> None:
    title, checklist = phase_checklist(role, mode, verify)
    source_label, source_items = source_checklist(role)
    status = "in_progress" if number == 1 else "pending"
    budget = max(1.0, hours / total)
    lines = [
        f"# Phase {number}/{total} — {title}",
        "",
        f"Status: `{status}` | Target duration: ~{budget:.1f} hours",
        "",
        "## Goal",
        "",
        f"- [ ] Complete the `{role}` phase without expanding its time box.",
        "",
        "## Governing checklist",
        "",
    ]
    lines.extend(f"- [ ] {item}" for item in checklist[:6])
    if source_items:
        lines.extend(["", f"### Generated from `llm/{source_label}`", ""])
        lines.extend(f"- [ ] {item}" for item in source_items)
    lines.extend(["", "## Phase checklist", ""])
    lines.extend(f"- [ ] {item}" for item in checklist[6:])
    lines.extend(
        [
            "",
            "## Exit criteria",
            "",
            "- [ ] Every checklist item has a recorded result and exact command/path where applicable.",
            "- [ ] The next phase has a concrete handoff, or this phase is ready for cleanup.",
            "",
        ]
    )
    path.write_text("\n".join(lines), encoding="utf-8")


def write_brief(
    path: Path,
    base: str,
    head: str,
    mode: str,
    review_modes: list[str],
    verify: list[str],
    hours: float,
    roles: list[str],
    changed: list[str],
    dirty: bool,
    report: str,
    patch_dir: str,
) -> None:
    generated = datetime.now(timezone.utc).isoformat(timespec="seconds")
    lines = [
        "# Review Session Brief",
        "",
        f"Generated: `{generated}`",
        f"Target: `{base}...{head}` (current branch upstream by default)",
        f"Output mode: `{mode}`",
        f"Review modes: `{', '.join(review_modes)}`",
        f"Verification: `{', '.join(verify)}`",
        f"Effort budget: `{hours:g}` hours; phase target: `1–3 hours`",
        f"Report: `{report}`",
        f"Patch directory: `{patch_dir}`",
        "",
        "## Contract",
        "",
        "- `commits`: one focused commit per accepted finding plus the report.",
        "- `patches`: one stable numbered patch per accepted finding plus the report.",
        "- `report`: report only; suggested fixes are fenced code blocks and no code is changed.",
        "- The preparation command never creates commits or patches; the LLM delivers them after review.",
        "",
        "## Changed footprint",
        "",
    ]
    lines.extend(f"- `{item}`" for item in changed or ["(no committed changes between revisions)"])
    lines.extend(
        [
            "",
            "## Phase plan",
            "",
            "| Phase | Role | Target |",
            "|---:|---|---:|",
        ]
    )
    budget = max(1.0, hours / len(roles))
    lines.extend(f"| {index} | `{role}` | ~{budget:.1f} h |" for index, role in enumerate(roles, 1))
    lines.extend(
        [
            "",
            "## Interview before review",
            "",
            "- Confirm output mode, review modes, target revisions, delivery scope, and selected verification.",
            "- If performance/benchmarking is selected, check PREWORK B/C before profiling.",
            "- If the target or scope is ambiguous, stop and ask; do not silently review the working tree.",
            "",
            "## Warnings",
            "",
            f"- Working tree has uncommitted/untracked changes outside the revision diff: `{dirty}`. They are excluded.",
            "- Missing upstream: rerun with `BASE=<revision>`.",
            "",
            "## Resume",
            "",
            "- Read this brief and the latest `bin/session_<YYYY-MM-DD>.md` before any new tool call.",
            "- Keep incomplete `bin/todo_<num>.md` files; delete only fully verified phase files.",
            "- Update the brief/session log after each verification boundary.",
            "",
        ]
    )
    path.write_text("\n".join(lines), encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mode", choices=sorted(OUTPUT_MODES), help="review output contract")
    parser.add_argument("--base", help="base revision; defaults to current branch upstream")
    parser.add_argument("--head", help="head revision; defaults to HEAD")
    parser.add_argument("--hours", help="estimated task hours; default 3")
    parser.add_argument("--phases", help="explicit phase count; otherwise derived from hours")
    parser.add_argument("--review-modes", help="comma-separated Immersion/Design/Performance modes")
    parser.add_argument("--verify", help="comma-separated checks: build/tests/python/snapshots/determinism/profiling")
    parser.add_argument("--output-dir", default=os.environ.get("OUTPUT_DIR") or "bin")
    parser.add_argument("--report", default=os.environ.get("REPORT") or "bin/report.md")
    parser.add_argument("--patch-dir", default=os.environ.get("PATCH_DIR") or "bin/patches")
    parser.add_argument("--force", action="store_true", help="replace existing numeric phase todos")
    args = parser.parse_args()
    if os.environ.get("FORCE"):
        args.force = True
    if sys.stdin.isatty() and not (args.mode and args.hours and args.review_modes and args.verify):
        print("make review — interactive. Shorthand values: output c/p/r; modes i/d/p; "
              "verify b/t/py/s/det/prof. Blank = default.")

    mode = choose_mode(args.mode)
    base = resolve_revision(args.base or os.environ.get("BASE"), "base")
    head = resolve_revision(args.head or os.environ.get("HEAD"), "head")
    hours = resolve_hours(args.hours)
    review_modes = resolve_review_modes(args.review_modes)
    verify = resolve_verify(args.verify)
    count = phase_count(mode, review_modes, hours, args.phases or os.environ.get("PHASES"))
    roles = phase_roles(mode, review_modes, count)

    output_dir = (ROOT / args.output_dir).resolve() if not Path(args.output_dir).is_absolute() else Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)
    existing = sorted(
        path
        for path in output_dir.glob("todo_[0-9]*.md")
        if re.fullmatch(r"todo_[1-9][0-9]*\.md", path.name)
    )
    if existing and not args.force:
        names = ", ".join(path.name for path in existing)
        raise SystemExit(f"Active phase todos already exist in {output_dir}: {names}. Resume them or use --force deliberately.")
    if args.force:
        for path in existing:
            path.unlink()

    changed_output = git("diff", "--name-status", "--find-renames", f"{base}...{head}")
    changed = [line for line in changed_output.splitlines() if line]
    dirty = bool(git("status", "--porcelain", check=False))
    report = args.report
    patch_dir = args.patch_dir
    brief_path = output_dir / "session_brief.md"
    write_brief(brief_path, base, head, mode, review_modes, verify, hours, roles, changed, dirty, report, patch_dir)
    for number, role in enumerate(roles, 1):
        write_phase(output_dir / f"todo_{number}.md", number, len(roles), role, mode, hours, verify)

    print(f"Review brief: {brief_path.relative_to(ROOT)}")
    print(f"Phase todos: {len(roles)} files, target ~{max(1.0, hours / len(roles)):.1f} hours each")
    print(f"Target: {base[:12]}...{head[:12]} | output: {mode} | verify: {','.join(verify)} | dirty excluded: {dirty}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
