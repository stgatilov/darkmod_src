import os, shutil, sys

def execute(cmd):
    print("CMD: " + cmd)
    assert os.system(cmd) == 0


assert sys.argv[1] == 'force', "This script should NOT be run locally!"

tdm_dir = os.path.basename(os.path.abspath(os.path.join(os.getcwd(), '..')))
tdm_copy_dir = tdm_dir + '_copy'

os.chdir('../..')

shutil.rmtree(f'{tdm_dir}/ThirdParty/artefacts')
shutil.copytree(tdm_dir, tdm_copy_dir)

os.environ['CONAN_HOME'] = os.path.abspath('conanhome')
os.chdir(f'{tdm_copy_dir}/CiScripts')

execute('python ./conan_rebuild_artefacts.py')

os.chdir('../..')
shutil.move(f'{tdm_copy_dir}/ThirdParty/artefacts', f'{tdm_dir}/ThirdParty/artefacts')
shutil.rmtree(tdm_copy_dir, ignore_errors = True)   # readonly .svn
shutil.rmtree('conanhome', ignore_errors = True)    # readonly cert?

os.environ['CONAN_HOME'] = 'wrong_conan_home_path'
os.chdir(f'{tdm_dir}/CiScripts')
