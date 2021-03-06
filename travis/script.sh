#! /bin/bash

if [ -z "$ANDROID_HOME" ]; then
  echo 'Building the Android parts of this project requires ANDROID_HOME'
  echo 'to point to the Android SDK, e.g. in $HOME/Android/Sdk'
fi

BAZEL_EXECUTABLE="${BAZEL_EXECUTABLE:-$(which bazel)}"
if [ -z "$BAZEL_EXECUTABLE" ]; then
  echo 'No bazel executable found or configured.'
  exit 1
fi

STRATEGY='--compilation_mode=opt'
STRATEGY+=' --verbose_failures'
if [ "$TRAVIS_OS_NAME" = linux -o "$(uname)" = Linux ]; then
  STRATEGY+=' --nodistinct_host_configuration'
  STRATEGY+=' --host_java_toolchain=@bazel_tools//tools/jdk:toolchain_hostjdk8'
  STRATEGY+=' --java_toolchain=@bazel_tools//tools/jdk:toolchain_hostjdk8'
fi
if [ -n "$TRAVIS" ]; then
  STRATEGY+=' --curses=no'
  #STRATEGY+=' --jobs=2'
  #STRATEGY+=' --local_resources=2048,.5,1.0'
  STRATEGY+=' --test_timeout_filters=-long'
fi

SHOW='--nocache_test_results --test_output=all'

set -o errexit
set -o xtrace
"$BAZEL_EXECUTABLE" info release
"$BAZEL_EXECUTABLE" run  $STRATEGY       //utils:python_version
"$BAZEL_EXECUTABLE" test $STRATEGY $SHOW //utils:python_version{,_sh}_test

"$BAZEL_EXECUTABLE" query '//... - rdeps(//..., filter("/textnorm/", //...))' |
  xargs "$BAZEL_EXECUTABLE" test $STRATEGY --
