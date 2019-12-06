import os, sys, glob

custom_recipes_list = glob.glob('custom/*/conanfile.py')
print("List of custom recipes found:")
print('\n'.join(['  ' + fn for fn in custom_recipes_list]))

conan_user_home = os.getenv('CONAN_USER_HOME')
print("Current conan cache: %s" % (conan_user_home if conan_user_home is not None else '[system-wide]'))
yn = input("Do you really want to export them all (yes/no): ")
if yn != 'yes':
    sys.exit(1)

for fn in custom_recipes_list:
    cmd = 'conan export %s thedarkmod/local' % fn
    print("CMD: %s" % cmd)
    ret = os.system(cmd)
