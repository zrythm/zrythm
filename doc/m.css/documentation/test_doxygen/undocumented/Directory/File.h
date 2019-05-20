/* An undocumented directory */

/* An undocumented file in an undocumented directory */

/* An undocumented class in the root namespace */
class Class {};

/* An undocumented structure in the root namespace */
struct Struct {};

/* An undocumented union in the root namespace */
union Union {};

/* An undocumented enum in the root namespace */
enum Enum {
    A,

    /**
     * This value is documented so the enum *should* be present in detailed
     * docs.
     */
    B,

    C
};

/* An undocumented typedef in the root namespace */
typedef int Int;

/* An undocumented using declaration in the root namespace */
using Float = float;

/* An undocumented variable in the root namespace */
constexpr const int Var = 3;

/* An undocumented function in the root namespace */
void foo();

/* An undocumented namespace */
namespace Namespace {

/* An undocumented class */
struct ClassInANamespace {
    /* An undocumented inner class */
    class ClassInAClass {};

    /* An undocumented inner enum */
    enum EnumInAClass {};

    /* An undocumented inner typedef */
    typedef int IntInAClass;

    /* An undocumented inner using declaration */
    using FloatInAClass = float;

    /* An undocumented inner variable */
    constexpr const int VarInAClass = 3;

    /* An undocumented inner function */
    void fooInAClass();
};

/* An undocumented enum */
enum EnumInANamespace {
    /* None of these are documented so the enum shouldn't be present in
       detailed docs */
    A, B, C
};

/* An undocumented typedef */
typedef int IntInANamespace;

/* An undocumented using declaration */
using FloatInANamespace = float;

/* An undocumented variable */
constexpr const int VarInANamespace = 3;

/* An undocumented function */
void fooInANamespace();

/** @{ @name A group */ /* does *not* contribute to search symbol count */

/* An undocumented flag in a group */
enum FlagInAGroup {};

/* An undocumented alias in a group */
using MainInAGroup = void;

/* A undocumented function in a group */
void barInAGroup();

/* An undocumented variable in a group */
constexpr void* variableInAGroup = nullptr;

/* An undocumented define in a group */
#define A_DEFINE_IN_A_GROUP

/*@}*/

/** @defgroup group A module
@{ */

/* An undocumented class in a module */
class ClassInModule {};

/* An undocumented structure in a module */
struct StructInModule {};

/* An undocumented union in a module */
union UnionInModule {};

/* An undocumented enum in a module */
enum EnumInModule {};

/* An undocumented typedef in a module */
typedef int IntInModule;

/* An undocumented using declaration in a module */
using FloatInModule = float;

/* An undocumented variable in a module */
constexpr const int VarInModule = 3;

/* An undocumented function in a module */
void fooInModule();

/*@}*/

}}

/* An undocumented define */
#define A_DEFINE
