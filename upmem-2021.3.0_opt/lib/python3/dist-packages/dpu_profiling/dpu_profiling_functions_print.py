from dpu_profiling import PrintResult, Duration

def parse_init():
    pass

def parse_end():
    pass

def print_summary_infos(infos, type_name):
        if (len(infos) == 0):
            return

        tot_duration = 0
        func_name = ""

        for xfer_info in infos:
                start_ns = xfer_info.start_ns
                end_ns = xfer_info.end_ns
                duration = end_ns - start_ns

                if func_name == "":
                    func_name = xfer_info.func_name.split("__")[1]

                tot_duration = tot_duration + duration

        print_duration = PrintResult.format_duration(tot_duration / len(infos))
        print_duration_tot = PrintResult.format_duration(tot_duration)
        pretty_type_name = func_name if type_name == "" else type_name
        print(PrintResult.GREEN + PrintResult.BOLD + "*** {:>40}:".format(pretty_type_name) + PrintResult.ENDC + "\tavg:{:12} tot:{:12} (nb: {})".format(print_duration, print_duration_tot, len(infos)))

def parse_unhandled(infos, pretty_type):
        print_summary_infos(infos, pretty_type)
