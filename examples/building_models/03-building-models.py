import stormpy
import stormpy.core

import stormpy.examples
import stormpy.examples.files


def example_building_models_01():
    path = stormpy.examples.files.prism_pdtmc_brp
    prism_program = stormpy.parse_prism_program(path)
    formula_str = "P=? [F s=5]"
    properties = stormpy.parse_properties_for_prism_program(formula_str, prism_program)

    options = stormpy.BuilderOptions([p.raw_formula for p in properties])
    options.set_build_state_valuations()
    model = stormpy.build_sparse_parametric_model_with_options(prism_program, options)
    eval2 = model.state_valuations.get_state(2)
    integer_variables = []
    boolean_variables = []
    for module in prism_program.modules:
        print("module {}".format(module.name))
        integer_variables += module.integer_variables
        boolean_variables += module.boolean_variables

    print(",".join(["{}: {}".format(str(iv.name), eval2.get_integer_value(iv.expression_variable)) for iv in integer_variables]))



if __name__ == '__main__':
    example_building_models_01()
