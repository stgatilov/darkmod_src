#ifndef __GLSL_DEF_H_4573873__
#define __GLSL_DEF_H_4573873__

/*
 * This header contains set of helpers to be used for GLSL packs.
 * Each pack is defined as a macro looking like this:
    #define UNIFORMS_GLOBAL(BEGIN, DEF, END) \
        BEGIN(Global, 10000) \
            DEF(ProjectionMatrix) \
            DEF(ModelViewMatrix) \
        END()
 * The technique called X-macro allows to generate various boilerplate code from it.
 */


/*
 * Declares a pack of uniforms in header file.
 * It declares a Enum and an Alias method in appropriate namespace.
 *
 * It is invoked as:
    UNIFORMS_GLOBAL(UNIFORMS_DECLARE_BEGIN, UNIFORMS_DECLARE_DEF, UNIFORMS_DECLARE_END)
 * It generates code:
    namespace Uniforms {
    	namespace Global {
    		enum Names {
                _ = 10000,
    			ProjectionMatrix,
    			ModelViewMatrix,
    		};
    		void Alias(GLSLProgram *program);
    	}
    }
 */
#define UNIFORMS_DECLARE_BEGIN(PackName, Start) \
    namespace Uniforms { \
        namespace PackName { \
            enum Names { \
                _ = Start,
#define UNIFORMS_DECLARE_DEF(Name) \
                Name,
#define UNIFORMS_DECLARE_END() \
            }; \
            void Alias(GLSLProgram *program); \
        } \
    }

/*
 * Defines Alias function for a pack of uniforms.
 *
 * It is invoked as:
    UNIFORMS_GLOBAL(UNIFORMS_ALIAS_BEGIN, UNIFORMS_ALIAS_DEF, UNIFORMS_ALIAS_END)
 * It generates code:
    void Uniforms::Global::Alias(GLSLProgram *program) {
        using namespace Uniforms::Global;
        program->AddUniformAlias(ProjectionMatrix, "u_" "ProjectionMatrix");
        program->AddUniformAlias(ModelViewMatrix, "u_" "ModelViewMatrix");
    }
 */
#define UNIFORMS_ALIAS_BEGIN(PackName, Start) \
    void Uniforms::PackName::Alias(GLSLProgram *program) { \
        using namespace Uniforms::PackName;
#define UNIFORMS_ALIAS_DEF(Name) \
        program->AddUniformAlias(Name, "u_" #Name);
#define UNIFORMS_ALIAS_END() \
    }

#endif
