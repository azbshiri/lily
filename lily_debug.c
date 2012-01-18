#include "lily_impl.h"
#include "lily_symtab.h"
#include "lily_opcode.h"

#define DEBUG_ARITH_OP(namestr, opstr) \
lily_impl_debugf(namestr, i); \
left = ((lily_sym *)code[i+1]); \
right = ((lily_sym *)code[i+2]); \
result = ((lily_sym *)code[i+3]); \
lily_impl_debugf("%s #%d " opstr " %s #%d to %s #%d\n", \
                 typename((lily_sym *)left), left->id, \
                 typename((lily_sym *)right), right->id, \
                 typename((lily_sym *)result), result->id); \
i += 4;

static char *typename(lily_sym *sym)
{
    char *ret;

    if (sym->flags & VAR_SYM)
        ret = "var";
    else if (sym->flags & LITERAL_SYM)
        ret = "literal";
    else if (sym->flags & STORAGE_SYM)
        ret = "storage";
    else
        ret = NULL;

    return ret;
}

static void show_code(lily_var *var)
{
    int i = 0;
    int len = ((lily_func_prop *)var->properties)->pos;
    int *code = ((lily_func_prop *)var->properties)->code;
    lily_sym *left, *right, *result;

    while (i < len) {
        switch (code[i]) {
            case o_assign:
                lily_impl_debugf("    [%d] assign        ", i);
                left = ((lily_sym *)code[i+1]);
                right = ((lily_sym *)code[i+2]);
                lily_impl_debugf("%s #%d to %s #%d\n",
                                 typename((lily_sym *)left),
                                 left->id, typename((lily_sym *)right),
                                 right->id);
                i += 3;
                break;
            case o_integer_add:
                DEBUG_ARITH_OP("    [%d] integer_add   ", "+")
                break;
            case o_integer_minus:
                DEBUG_ARITH_OP("    [%d] integer_minus ", "-")
                break;
            case o_number_add:
                DEBUG_ARITH_OP("    [%d] number_add    ", "+")
                break;
            case o_number_minus:
                DEBUG_ARITH_OP("    [%d] number_minus  ", "-")
                break;
            case o_func_call:
                {
                int j;
                lily_var *var;

                /* var, func, #args, args... */
                var = (lily_var *)code[i+1];
                lily_impl_debugf("    [%d] func_call  \n        ", i);

                if (var->line_num == 0)
                    lily_impl_debugf("name: (builtin) %s.\n        args:\n",
                                     var->name, code[i+3]);
                else
                    lily_impl_debugf("name: %s (from line %d).\n        args:\n",
                                     var->name, var->line_num, code[i+3]);

                for (j = 0;j < code[i+3];j++) {
                    lily_sym *sym = (lily_sym *)code[i+4+j];
                    lily_impl_debugf("            #%d: %s #%d\n", j+1,
                                     typename(sym), sym->id);
                }

                i += 4 + j;
                }
                break;
            case o_vm_return:
                lily_impl_debugf("    [%d] vm_return\n", i);
                return;
            default:
                lily_impl_debugf("    [%d] bad opcode %d.\n", i, code[i]);
                return;
        }
    }
}

static void show_sym(lily_sym *sym)
{
    if (!(sym->flags & S_IS_NIL)) {
        if (sym->sig->cls->id == SYM_CLASS_STR) {
            lily_impl_debugf("str(%-0.50s)\n",
                            ((lily_strval *)sym->value.ptr)->str);
        }
        else if (sym->sig->cls->id == SYM_CLASS_INTEGER)
            lily_impl_debugf("integer(%d)\n", sym->value.integer);
        else if (sym->sig->cls->id == SYM_CLASS_NUMBER)
            lily_impl_debugf("number(%f)\n", sym->value.number);
    }
    else
        lily_impl_debugf("(nil)\n");
}

void lily_show_var_values(lily_symtab *symtab)
{
    lily_var *var = symtab->var_start;

    while (var != NULL && var->line_num == 0)
        var = var->next;

    if (var != NULL) {
        lily_impl_debugf("Var values:\n");

        while (var != NULL) {
            lily_impl_debugf("%s %s = ", var->sig->cls->name, var->name);
            show_sym((lily_sym *)var);
            var = var->next;
        }
    }
}

void lily_show_symtab(lily_symtab *symtab)
{
    lily_var *var = symtab->var_start;
    lily_literal *lit = symtab->lit_start;

    /* The only classes now are the builtin ones. */
    lily_impl_debugf("Classes:\n");
    int class_i;
    for (class_i = 0;class_i <= SYM_CLASS_FUNCTION;class_i++)
        lily_impl_debugf("#%d: (builtin) %s\n", class_i, symtab->classes[class_i]->name);

    lily_impl_debugf("Literals:\n");
    /* Show literal values first. */
    while (lit != NULL) {
        lily_impl_debugf("#%d: ", lit->id);
        show_sym((lily_sym *)lit);
        lit = lit->next;
    }

    lily_impl_debugf("Vars:\n");
    while (var != NULL) {
        lily_impl_debugf("#%d: ", var->id);
        if (var->line_num == 0) {
            /* This is a builtin symbol. */
            lily_impl_debugf("(builtin) %s %s\n", var->sig->cls->name,
                             var->name);
            if (isafunc(var) && ((lily_func_prop *)var->properties)->code != NULL)
                show_code(var);
        }
        else {
            lily_impl_debugf("%s %s @ line %d\n", var->sig->cls->name,
                             var->name, var->line_num);
        }
        var = var->next;
    }
}
