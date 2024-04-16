#ifndef __STATE_MACHINE_H__
#define __STATE_MACHINE_H__


/*
 * Defines the state of the PE. The state machine of a PREM task is the following:
 *                 ┌──────────────────────> Suspended
 *  request memory │                          ￪  │
 *      denied     │                IPI pause │  │ IPI resume
 *                 │                          │  ￬
 *              (Ready) ──────────────────> Memory phase
 *                 ￪     request memory         │
 *       Task      │        granted             │   revoke memory
 *     released    │                            │      access
 *                 │                            ￬
 *        ────> Waiting <─────────────── Computation phase
 *                            done
 */
enum states
{
    WAITING,
    SUSPENDED,
    MEMORY_PHASE,
    COMPUTATION_PHASE
};

/* Change state */
void change_state(enum states new_state);

/* Get current state */
enum states get_current_state();

#endif