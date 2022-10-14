#!/bin/sh -e
name=${CONTAINER_IMAGE}/server

# build and publish Docker image (gitlab-ci)

docker build . \
	-t ${name}:${CI_COMMIT_SHA} \
	-t ${name}:${CI_COMMIT_REF_NAME} \
	-t ${name}:latest

docker push ${name}:${CI_COMMIT_SHA}
docker push ${name}:${CI_COMMIT_REF_NAME}
[ "$CI_COMMIT_BRANCH" = master ] && docker push ${name}:latest

exit 0
