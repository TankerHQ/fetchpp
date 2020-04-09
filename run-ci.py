import argparse
import os
import sys

from path import Path

import ci
import ci.cpp
import ci.conan
import ci.git


def build_and_test(profile: str, coverage: bool) -> None:
    built_path = ci.cpp.build(profile, make_package=True, coverage=coverage)
    ci.cpp.check(built_path, coverage=coverage)


def deploy() -> None:
    git_tag = os.environ["CI_COMMIT_TAG"] 
    version = ci.version_from_git_tag(git_tag)
    ci.bump_files(version)
    ci.cpp.build_recipe(
        Path.getcwd(),
        conan_reference=f"fetchpp/{version}@tanker/stable",
        upload=True
    )


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--isolate-conan-user-home", action="store_true", dest="home_isolation", default=False)
    subparsers = parser.add_subparsers(title="subcommands", dest="command")

    build_and_test_parser = subparsers.add_parser("build-and-test")
    build_and_test_parser.add_argument("--profile", required=True)
    build_and_test_parser.add_argument("--coverage", action="store_true")

    subparsers.add_parser("deploy")
    subparsers.add_parser("mirror")

    ci.conan.update_config()

    args = parser.parse_args()
    if args.home_isolation:
        ci.conan.set_home_isolation()

    if args.command == "build-and-test":
        build_and_test(args.profile, args.coverage)
    elif args.command == "deploy":
        deploy()


if __name__ == "__main__":
    main()
