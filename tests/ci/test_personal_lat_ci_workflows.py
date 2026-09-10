#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Tests for the personal-fork LAT CI workflow security helpers."""

import importlib.util
import io
import json
import os
import subprocess
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
PERSONAL_REPOSITORY = "xiezyang/lat"
HEAD_SHA = "a" * 40
BASE_SHA = "b" * 40


def load_helper(name):
    path = ROOT / "scripts" / "ci" / f"{name}.py"
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


REQUEST = load_helper("lat_ci_manual_request")
CALLBACK = load_helper("lat_ci_callback")


def pull_request(target=PERSONAL_REPOSITORY, head_sha=HEAD_SHA):
    return {
        "number": 42,
        "state": "open",
        "head": {
            "sha": head_sha,
            "ref": "ci/security-port",
            "repo": {"full_name": PERSONAL_REPOSITORY, "id": 12345},
        },
        "base": {"sha": BASE_SHA, "repo": {"full_name": target}},
    }


class Response(io.BytesIO):
    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        return False


class ManualRequestTest(unittest.TestCase):
    def resolve(self, pr, *, target=PERSONAL_REPOSITORY):
        return REQUEST.resolve_pull_request(
            "42", target, lambda path: pr
        )

    def test_accepts_the_personal_repository_target(self):
        personal = self.resolve(pull_request())
        self.assertEqual(personal["target_repository"], PERSONAL_REPOSITORY)

    def test_rejects_a_target_other_than_the_personal_repository(self):
        with self.assertRaisesRegex(REQUEST.LatCIRequestError, "must run in"):
            self.resolve(pull_request(), target="lat-opensource/lat")

    def test_resolves_current_head(self):
        self.assertEqual(self.resolve(pull_request(head_sha="c" * 40))["head_sha"], "c" * 40)

    def test_rejects_closed_pr_wrong_base_unavailable_head_and_external_source(self):
        closed = pull_request()
        closed["state"] = "closed"
        with self.assertRaisesRegex(REQUEST.LatCIRequestError, "must be open"):
            self.resolve(closed)

        wrong_base = pull_request()
        wrong_base["base"]["repo"]["full_name"] = "example/lat"
        with self.assertRaisesRegex(REQUEST.LatCIRequestError, "base repository"):
            self.resolve(wrong_base)

        missing_head = pull_request()
        missing_head["head"]["repo"] = None
        with self.assertRaisesRegex(REQUEST.LatCIRequestError, "head repository is unavailable"):
            self.resolve(missing_head)

        external_source = pull_request()
        external_source["head"]["repo"]["full_name"] = "reviewers/test-suite"
        with self.assertRaisesRegex(REQUEST.LatCIRequestError, "head must be"):
            self.resolve(external_source)

    def test_uses_github_token_for_api_access(self):
        requests = []

        def opener(request, timeout):
            requests.append(request)
            return Response(json.dumps({"ok": True}).encode("utf-8"))

        self.assertEqual(REQUEST.get_json("repos/xiezyang/lat", "test-token", opener), {"ok": True})
        self.assertEqual(requests[0].get_header("Authorization"), "Bearer test-token")


class CallbackTest(unittest.TestCase):
    def setUp(self):
        self.secret = "test-callback-secret"
        self.timestamp = "1700000000"
        self.values = {
            "repository": PERSONAL_REPOSITORY,
            "head_sha": HEAD_SHA,
            "conclusion": "success",
            "summary": "All selected LAT CI suites completed successfully.",
            "details_url": "https://github.com/xiezyang/lat-ci-hub/actions/runs/1",
        }

    def test_accepts_only_a_valid_hub_signature(self):
        signature = CALLBACK.create_signature(
            self.secret, timestamp=int(self.timestamp), **self.values
        )
        CALLBACK.verify_callback(
            self.secret,
            timestamp_value=self.timestamp,
            signature=signature,
            now=int(self.timestamp) + 10,
            **self.values,
        )
        with self.assertRaisesRegex(CALLBACK.LatCICallbackError, "signature"):
            CALLBACK.verify_callback(
                self.secret,
                timestamp_value=self.timestamp,
                signature="sha256=" + "0" * 64,
                now=int(self.timestamp) + 10,
                **self.values,
            )


class WorkflowDefinitionTest(unittest.TestCase):
    def test_dispatch_defaults_and_explicit_request(self):
        workflow = (ROOT / ".github/workflows/request-lat-ci.yml").read_text()
        # Execute the actual payload builder embedded in the workflow.
        import textwrap
        code = textwrap.dedent(workflow.split("<<'PY'\n", 1)[1].split("\n          PY", 1)[0])
        environment = dict(os.environ, **{name: "test" for name in (
            "REQUEST_KIND", "SOURCE_REPOSITORY", "SOURCE_REPOSITORY_ID",
            "TARGET_REPOSITORY", "PR_NUMBER", "HEAD_SHA", "BASE_SHA",
            "HEAD_REF", "REQUESTER",
        )})
        explicit = '{"schema_version":1,"suites":[]}'
        for value in ("", "   ", explicit):
            environment["TEST_REQUEST"] = value
            payload = json.loads(subprocess.check_output(
                ["python3", "-c", code], env=environment, text=True
            ))
            request = payload["inputs"]["test_request"]
            if value == explicit:
                self.assertEqual(request, explicit)
            else:
                self.assertEqual(json.loads(request), {
                    "schema_version": 1,
                    "suites": [{"suite_key": "spec2000", "parameters": {
                        "input_size": "ref", "runs_per_invocation": 3,
                        "test_set": "all", "times": 2,
                        "performance_analysis": True,
                    }, "environment": {}}],
                })

    def test_manual_request_passes_the_approved_sha_and_token_to_the_helper(self):
        workflow = (ROOT / ".github" / "workflows" / "request-lat-ci.yml").read_text(
            encoding="utf-8"
        )
        self.assertNotIn("expected_head_sha:", workflow)
        self.assertNotIn("EXPECTED_HEAD_SHA", workflow)
        self.assertIn("GITHUB_TOKEN: ${{ github.token }}", workflow)
        self.assertIn("scripts/ci/lat_ci_manual_request.py", workflow)
        self.assertIn("actions/checkout@v5", workflow)
        self.assertNotIn("pull_request:", workflow)
        self.assertNotIn("push:", workflow)
        self.assertIn('"request_kind": "manual"', workflow)

    def test_callback_authentication_precedes_check_run_creation(self):
        workflow = (ROOT / ".github" / "workflows" / "lat-ci-check.yml").read_text(
            encoding="utf-8"
        )
        self.assertIn("callback_timestamp:", workflow)
        self.assertIn("callback_signature:", workflow)
        self.assertIn("LAT_CI_CHECK_CALLBACK_SECRET", workflow)
        self.assertIn("scripts/ci/lat_ci_callback.py", workflow)
        self.assertLess(
            workflow.index("Authenticate the LAT CI hub callback"),
            workflow.index("Create Check Run on the tested LAT commit"),
        )


if __name__ == "__main__":
    unittest.main()
