import os, shutil

def execute(cmd):
    print("CMD: " + cmd)
    assert os.system(cmd) == 0


execute('conan config install global.conf')

try:
    bitnessArg = '--bitness=' + os.environ['PLATFORM']
except:
    bitnessArg = ""

print('CONAN_HOME = ' + os.environ['CONAN_HOME'])

os.chdir('../ThirdParty')

shutil.rmtree('artefacts', ignore_errors = True)
execute('python ./1_export_custom.py --unattended')
execute(f'python ./2_build_all.py --unattended {bitnessArg}')

os.chdir('../CiScripts')
