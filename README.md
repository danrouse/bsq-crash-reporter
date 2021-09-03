# Crash Reporter for Beat Saber for Quest

This multi-part repo includes the full stack around the crash-reporter Quest mod, which detects and uploads tombstones generated from game crashes.

- `qmod`, containing the Quest mod source and build pipeline,
- `server`, containing the (Node.js) server,
- a variety of scripts for obtaining and generating debug versions of libraries for `ndk-stack` usage on the server, for more detailed crash logs on the frontend

## Infrastructure notes
- The web service is currently hosted on Google Cloud Platform.
- When updating, the server Docker container gets built and pushed to GCR (Google Container Registry) and then triggers GCE (Google Cloud Engine) to restart the service with the new container.
- DNS is currently handled by the mods.quest registrar, Gandi.
- Data is stored on Google Firestore. Authorization is through a GCP IAM role. Credentials are stored in `server/firestore-key.json` which is read at runtime.

## wild ideas
- somehow get any available debug builds of mods that aren't on qpm
- in qmod: allow user to enable logging and reproduce crash, then upload log
