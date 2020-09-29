import argparse


import tankerci
import tankerci.cpp
import tankerci.conan
import tankerci.git


def build_and_test(profile: str, coverage: bool) -> None:
    built_path = tankerci.cpp.build(profile, make_package=True, coverage=coverage)
    tankerci.cpp.check(built_path, coverage=coverage)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--isolate-conan-user-home",
        action="store_true",
        dest="home_isolation",
        default=False,
    )
    subparsers = parser.add_subparsers(title="subcommands", dest="command")

    build_and_test_parser = subparsers.add_parser("build-and-test")
    build_and_test_parser.add_argument("--profile", required=True)
    build_and_test_parser.add_argument("--coverage", action="store_true")

    subparsers.add_parser("deploy")
    subparsers.add_parser("mirror")

    tankerci.conan.update_config()

    args = parser.parse_args()
    if args.home_isolation:
        tankerci.conan.set_home_isolation()

    if args.command == "build-and-test":
        build_and_test(args.profile, args.coverage)
    elif args.command == "mirror":
        tankerci.git.mirror(github_url="git@github.com:TankerHQ/fetchpp", force=True)


if __name__ == "__main__":
    main()
