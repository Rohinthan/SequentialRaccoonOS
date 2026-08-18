#define _POSIX_C_SOURCE 200809L

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "scrpd_service.h"
#include "scrpd_supervised_start.h"
#include "scrpd_supervisor.h"
#include "scrpd_supervisor_recover.h"
#include "scrpd_runtime_state.h"

static volatile sig_atomic_t running = 1;

static const char *STATE_DIRECTORY =
	"service-manager/state";

static void handle_signal(int signal)
{
    (void)signal;
    running = 0;
}

int main(int argc, char **argv)
{
    if (argc != 3) {
        fprintf(
            stderr,
            "Usage:\n"
            "  scrpd-supervise <root-directory> <service-file>\n"
        );

        return 1;
    }

    const char *root = argv[1];
    const char *service_file = argv[2];

    scrpd_service_t service;

    if (scrpd_service_load(
            service_file,
            &service
        ) != 0) {

        fprintf(
            stderr,
            "SCRPD-SUPERVISE: failed to load service\n"
        );

        return 1;
    }

    scrpd_runtime_t runtime;

    scrpd_runtime_init(
        &runtime,
        service.name
    );

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    printf("SCRPD SUPERVISOR\n");
    printf("Name:    %s\n", service.name);
    printf("Command: %s\n", service.command);
    printf("Root:    %s\n", root);

int restored = 0;

if (scrpd_runtime_state_load(
        &runtime,
        "service-manager/state"
    ) == 0) {

    printf("SCRPD-SUPERVISE: persisted state loaded\n");

    printf(
        "  Name:  %s\n"
        "  PID:   %ld\n"
        "  State: %s\n",
        runtime.runtime.name,
        runtime.runtime.pid,
        scrpd_state_name(runtime.runtime.state)
    );

    if (scrpd_runtime_is_alive(&runtime)) {
        restored = 1;

        printf(
            "SCRPD-SUPERVISE: existing service is alive\n"
        );
    } else {
        runtime.runtime.state = SCRPD_STATE_FAILED;

        printf(
            "SCRPD-SUPERVISE: persisted PID is stale\n"
        );
    }
    }

    if (!restored &&
        runtime.runtime.state != SCRPD_STATE_FAILED) {

    if (scrpd_supervised_start(
            &runtime,
            &service,
            root
        ) != 0) {

        fprintf(
            stderr,
            "SCRPD-SUPERVISE: initial service start failed\n"
        );

        return 1;
     }
     }

   if (scrpd_runtime_state_save(
	&runtime,
	STATE_DIRECTORY
      ) != 0) {

      fprintf(
	 stderr,
	 "SCRPD-SUPERVISE: failed to save recovered state\n"
      );

      return 1;

   }

    printf("Supervisor loop started\n");

    while (running) {

        if (scrpd_supervisor_reconcile(
                &runtime
            ) != 0) {

            fprintf(
                stderr,
                "SCRPD-SUPERVISE: reconciliation failed\n"
            );

            break;
        }

        int alive = scrpd_runtime_is_alive(
            &runtime
        );

        printf(
            "SCRPD-SUPERVISE: state=%s alive=%s\n",
            scrpd_state_name(
                runtime.runtime.state
            ),
            alive ? "yes" : "no"
        );

        if (runtime.runtime.state == SCRPD_STATE_FAILED) {

            printf(
                "SCRPD-SUPERVISE: service failure detected\n"
            );

            if (scrpd_supervisor_recover(
                    &runtime,
                    &service,
                    root
                ) != 0) {

                fprintf(
                    stderr,
                    "SCRPD-SUPERVISE: recovery failed\n"
                );

                break;
            }

            printf(
                "SCRPD-SUPERVISE: service recovered\n"
            );
        }

        sleep(1);
    }

    printf("SCRPD SUPERVISOR: stopped\n");

    return 0;
}
