#!bin/bash

SCRIPT_DIR="$(readlink -f "$(dirname "${BASH_SOURCE[0]}")")"

echo "Enter UPMEM_HOME directory (default /home/siung/reduce_scatter/upmem-2021.3.0_opt): "
read upmem_home_dir
if [ "${upmem_home_dir}" = "" ]; then
    export UPMEM_HOME="/home/joungwook/PID-Comm/upmem-2021.3.0_opt" #directory of UPMEM SDK. Copy only necessary modified .so files built inside the repo for now - Comment
else
    export UPMEM_HOME=$upmem_home_dir
fi

#export UPMEM_HOME="${SCRIPT_DIR}"
echo "Setting UPMEM_HOME to ${UPMEM_HOME} and updating PATH/LD_LIBRARY_PATH/PYTHONPATH"
export LD_LIBRARY_PATH="${UPMEM_HOME}/lib${LD_LIBRARY_PATH+:$LD_LIBRARY_PATH}"
export PATH="${UPMEM_HOME}/bin:${PATH}"

PYTHON_RELATIVE_PATH=$(/usr/bin/env python3 -c "import distutils.sysconfig; print(distutils.sysconfig.get_python_lib(True, False, ''))")

if [[ -z "${PYTHON_RELATIVE_PATH}" ]];
then
    echo "Could not set PYTHONPATH!"
else
    export PYTHONPATH="${UPMEM_HOME}/${PYTHON_RELATIVE_PATH}${PYTHONPATH+:$PYTHONPATH}"
fi

_DEFAULT_BACKEND=$1

if [[ -n "${_DEFAULT_BACKEND}" ]];
then
    echo "Setting default backend to ${_DEFAULT_BACKEND} in UPMEM_PROFILE_BASE"
    export UPMEM_PROFILE_BASE=backend=${_DEFAULT_BACKEND}
fi
