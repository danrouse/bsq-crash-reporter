# Crash Reporter for Beat Saber for Quest

This multi-part repo includes the full stack around the crash-reporter Quest mod, which detects and uploads tombstones generated from game crashes. There are four components:

- `qmod`, containing the Quest mod source and build pipeline,
- `server`, containing the (Node.js) server,
- a variety of scripts for obtaining and generating debug versions of libraries for `ndk-stack` usage on the server, for more detailed crash logs on the frontend, as well as Docker build and development scripts,
- and finally, not included in the repo, AWS infrastructure, including server (Elastic Beanstalk using the Docker image), database (Postgres on RDS), and storage (S3)
