#include <asm/asm.h>
#include <linux/linkage.h>
#include "../include/vcpu_run.h"

/*
 * Since *vmsave/vmload* and *vmrun/vmexit* only manage
 * system state like *control registers* and *segments*,
 * so we should save and restore other states, such as
 * general-purpose registers.
 *
 * %rdi is vcpu->gctx
 * %rsi is vcpu->hctx
 * %rdx is virt_to_phys(vcpu->gctx.vmcb)
 */
SYM_FUNC_START(_yakvm_vcpu_run)

/* save host registers */
mov %rbx, YAKVM_VCPU_CONTEXT_RBX_OFF(%rsi)
mov %rcx, YAKVM_VCPU_CONTEXT_RCX_OFF(%rsi)
mov %rbp, YAKVM_VCPU_CONTEXT_RBP_OFF(%rsi)
mov %r8, YAKVM_VCPU_CONTEXT_R8_OFF(%rsi)
mov %r9, YAKVM_VCPU_CONTEXT_R9_OFF(%rsi)
mov %r10, YAKVM_VCPU_CONTEXT_R10_OFF(%rsi)
mov %r11, YAKVM_VCPU_CONTEXT_R11_OFF(%rsi)
mov %r12, YAKVM_VCPU_CONTEXT_R12_OFF(%rsi)
mov %r13, YAKVM_VCPU_CONTEXT_R13_OFF(%rsi)
mov %r14, YAKVM_VCPU_CONTEXT_R14_OFF(%rsi)
mov %r15, YAKVM_VCPU_CONTEXT_R15_OFF(%rsi)
push %rsi   /* save vcpu->hctx */
push %rdi   /* save vcpu->gctx */
push %rdx   /* save virt_to_phys(vcpu->gctx.vmcb) */

/* load guest registers */
mov %rdi, %rax /* use rax as base for vcpu->hctx */
mov YAKVM_VCPU_CONTEXT_RBX_OFF(%rax), %rbx
mov YAKVM_VCPU_CONTEXT_RCX_OFF(%rax), %rcx
mov YAKVM_VCPU_CONTEXT_RDX_OFF(%rax), %rdx
mov YAKVM_VCPU_CONTEXT_RDI_OFF(%rax), %rdi
mov YAKVM_VCPU_CONTEXT_RSI_OFF(%rax), %rsi
mov YAKVM_VCPU_CONTEXT_RBP_OFF(%rax), %rbp
mov YAKVM_VCPU_CONTEXT_R8_OFF(%rax), %r8
mov YAKVM_VCPU_CONTEXT_R9_OFF(%rax), %r9
mov YAKVM_VCPU_CONTEXT_R10_OFF(%rax), %r10
mov YAKVM_VCPU_CONTEXT_R11_OFF(%rax), %r11
mov YAKVM_VCPU_CONTEXT_R12_OFF(%rax), %r12
mov YAKVM_VCPU_CONTEXT_R13_OFF(%rax), %r13
mov YAKVM_VCPU_CONTEXT_R14_OFF(%rax), %r14
mov YAKVM_VCPU_CONTEXT_R15_OFF(%rax), %r15

/* run vm */
pop %rax /* load virt_to_phys(vcpu->gctx.vmcb) */
vmload %rax
vmrun %rax
vmsave %rax

/* save guest registers */
pop %rax /* load vcpu->gctx */
mov %rbx, YAKVM_VCPU_CONTEXT_RBX_OFF(%rax)
mov %rcx, YAKVM_VCPU_CONTEXT_RCX_OFF(%rax)
mov %rdx, YAKVM_VCPU_CONTEXT_RDX_OFF(%rax)
mov %rdi, YAKVM_VCPU_CONTEXT_RDI_OFF(%rax)
mov %rsi, YAKVM_VCPU_CONTEXT_RSI_OFF(%rax)
mov %rbp, YAKVM_VCPU_CONTEXT_RBP_OFF(%rax)
mov %r8, YAKVM_VCPU_CONTEXT_R8_OFF(%rax)
mov %r9, YAKVM_VCPU_CONTEXT_R9_OFF(%rax)
mov %r10, YAKVM_VCPU_CONTEXT_R10_OFF(%rax)
mov %r11, YAKVM_VCPU_CONTEXT_R11_OFF(%rax)
mov %r12, YAKVM_VCPU_CONTEXT_R12_OFF(%rax)
mov %r13, YAKVM_VCPU_CONTEXT_R13_OFF(%rax)
mov %r14, YAKVM_VCPU_CONTEXT_R14_OFF(%rax)
mov %r15, YAKVM_VCPU_CONTEXT_R15_OFF(%rax)

/* load host registers except %rdi and %rdx */
pop %rsi        /* load vcpu->hctx */
mov YAKVM_VCPU_CONTEXT_R15_OFF(%rsi), %r15
mov YAKVM_VCPU_CONTEXT_R14_OFF(%rsi), %r14
mov YAKVM_VCPU_CONTEXT_R13_OFF(%rsi), %r13
mov YAKVM_VCPU_CONTEXT_R12_OFF(%rsi), %r12
mov YAKVM_VCPU_CONTEXT_R11_OFF(%rsi), %r11
mov YAKVM_VCPU_CONTEXT_R10_OFF(%rsi), %r10
mov YAKVM_VCPU_CONTEXT_R9_OFF(%rsi), %r9
mov YAKVM_VCPU_CONTEXT_R8_OFF(%rsi), %r8
mov YAKVM_VCPU_CONTEXT_RBP_OFF(%rsi), %rbp
mov YAKVM_VCPU_CONTEXT_RCX_OFF(%rsi), %rcx
mov YAKVM_VCPU_CONTEXT_RBX_OFF(%rsi), %rbx

RET
SYM_FUNC_END(_yakvm_vcpu_run)
