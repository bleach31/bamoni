"""Bamoni CI pipeline."""

import dagger
from dagger import dag, function, object_type


@object_type
class Bamoni:
    @function
    async def build_docs(self, source: dagger.Directory) -> dagger.Directory:
        """Build Sphinx documentation and return the output directory."""
        return await (
            dag.container()
            .from_("python:3.10-slim")
            .with_directory("/src", source)
            .with_workdir("/src")
            .with_exec(["pip", "install", "-r", "docs/requirements.txt"])
            .with_exec(["sphinx-build", "-b", "html", "docs/source", "docs/build"])
            .directory("/src/docs/build")
        )

    @function
    async def build_firmware(
        self,
        source: dagger.Directory,
        project: str = "firmware",
        environment: str = "",
    ) -> str:
        """Build PlatformIO firmware for the specified project and environment."""
        workdir = f"/src/impl/{project}"
        ctr = (
            dag.container()
            .from_("python:3.11-slim")
            .with_exec(["pip", "install", "platformio"])
            .with_directory("/src", source)
            .with_workdir(workdir)
        )
        cmd = ["pio", "run"]
        if environment:
            cmd += ["-e", environment]
        return await ctr.with_exec(cmd).stdout()
