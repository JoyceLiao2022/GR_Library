import gr_python

gsrs = gr_python.compute_gsrs_from_file("tests/data/toy.log")
gnrs = gr_python.compute_gnrs_from_file("tests/data/toy.log")

print("GSRs:")

for reason in gsrs:
    print([
        (assignment.variable, assignment.states)
        for assignment in reason
    ])

print("GNRs:")

for reason in gnrs:
    print([
        (assignment.variable, assignment.states)
        for assignment in reason
    ])