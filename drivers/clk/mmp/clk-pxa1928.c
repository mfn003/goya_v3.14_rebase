#include <linux/io.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/clkdev.h>
#include <linux/of.h>
#include <linux/of_address.h>

#include <dt-bindings/clock/marvell-pxa1928.h>

#include "clk.h"
#include "clk-pll-pxa1928.h"
#include "clk-core-pxa1928.h"

#define APBC_RTC		0x0
#define APBC_TWSI0		0x4
#define APBC_TWSI1		0x8
#define APBC_TWSI2		0xc
#define APBC_TWSI3		0x10
#define APBC_TWSI4		0x7c
#define APBC_TWSI5		0x80
#define APBC_KPC		0x18
#define APBC_UART0		0x2c
#define APBC_UART1		0x30
#define APBC_UART2		0x34
#define APBC_UART3		0x88
#define APBC_GPIO		0x38
#define APBC_PWM0		0x3c
#define APBC_PWM1		0x40
#define APBC_PWM2		0x44
#define APBC_PWM3		0x48

#define MPMU_UART_PLL		0x14

#define APMU_SDH0		0x54
#define APMU_SDH1		0x58
#define APMU_SDH2		0xe8
#define APMU_SDH3		0xec
#define APMU_SDH4		0x15c
#define APMU_USB		0x5c
#define APMU_APDBG		0x340
#define APMU_AP_DEBUG1		0x38c

/* PLL2~PLL7 related */
#define POSR_PLL2_LOCK		(1 << 29)
#define POSR_PLL3_LOCK		(1 << 14)
#define POSR_PLL4_LOCK		(1 << 15)
#define POSR_PLL5_LOCK		(1 << 16)
#define POSR_PLL6_LOCK		(1 << 14)
#define POSR_PLL7_LOCK		(1 << 17)

#define APMU_GLB_CLK_CTRL	(0x0dc)
#define APMU_MC_CLK_RES_CTRL	(0x258)
#define MPMU_PLL2CR	(0x0034)
#define MPMU_PLL3CR	(0x0050)
#define MPMU_PLL4CR	(0x1100)
#define MPMU_PLL5CR	(0x1114)
#define MPMU_PLL6CR	(0x1300)
#define MPMU_PLL7CR	(0x1400)
#define MPMU_POSR	(0x0010)
#define MPMU_POSR2	(0x0054)
#define MPMU_POSR3	(0x0074)
#define MPMU_PLL2_CTRL1	(0x0414)
#define MPMU_PLL3_CTRL1	(0x0058)
#define MPMU_PLL4_CTRL1	(0x1104)
#define MPMU_PLL5_CTRL1	(0x1118)
#define MPMU_PLL6_CTRL1	(0x1304)
#define MPMU_PLL7_CTRL1	(0x1404)
#define MPMU_PLL2_CTRL2	(0x0418)
#define MPMU_PLL3_CTRL2	(0x0060)
#define MPMU_PLL4_CTRL2	(0x1108)
#define MPMU_PLL5_CTRL2	(0x111c)
#define MPMU_PLL6_CTRL2	(0x1308)
#define MPMU_PLL7_CTRL2	(0x1408)
#define MPMU_PLL2_CTRL3	(0x041c)
#define MPMU_PLL3_CTRL3	(0x0064)
#define MPMU_PLL4_CTRL3	(0x110c)
#define MPMU_PLL5_CTRL3	(0x1120)
#define MPMU_PLL6_CTRL3	(0x130c)
#define MPMU_PLL7_CTRL3	(0x140c)
#define MPMU_PLL2_CTRL4	(0x0068)
#define MPMU_PLL3_CTRL4	(0x006c)
#define MPMU_PLL4_CTRL4	(0x1110)
#define MPMU_PLL5_CTRL4	(0x1124)
#define MPMU_PLL6_CTRL4	(0x1310)
#define MPMU_PLL7_CTRL4	(0x1410)
#define APMU_AP_PLL_CTRL	(0x0348)
#define GATE_CTRL_SHIFT_PLL2	(22)
#define GATE_CTRL_SHIFT_PLL3	(24)
#define GATE_CTRL_SHIFT_PLL4	(26)
#define GATE_CTRL_SHIFT_PLL5	(28)
#define GATE_CTRL_SHIFT_PLL6	(30)
#define GATE_CTRL_SHIFT_PLL7	(16)
#define APMU_DISP_RST_CTRL	(0x180)
#define APMU_DISP_CLK_CTRL	(0x184)
#define APMU_DISP_CLK_CTRL2	(0x188)

struct pxa1928_clk_unit {
	struct mmp_clk_unit unit;
	void __iomem *mpmu_base;
	void __iomem *apmu_base;
	void __iomem *apbc_base;
	void __iomem *ciu_base;
};

static struct mmp_param_fixed_rate_clk fixed_rate_clks[] = {
	{PXA1928_CLK_CLK32, "clk32", NULL, CLK_IS_ROOT, 32768},
	{PXA1928_CLK_VCTCXO, "vctcxo", NULL, CLK_IS_ROOT, 26000000},
	{PXA1928_CLK_PLL1_624, "pll1_624", NULL, CLK_IS_ROOT, 624000000},
	{PXA1928_CLK_PLL1_416, "pll1_416", NULL, CLK_IS_ROOT, 416000000},
	{PXA1928_CLK_USB_PLL, "usb_pll", NULL, CLK_IS_ROOT, 480000000},
};

static struct mmp_param_fixed_factor_clk fixed_factor_clks[] = {
	{PXA1928_CLK_VCTCXO_2, "vctcxo_2", "vctcxo", 1, 2, 0},
	{PXA1928_CLK_VCTCXO_4, "vctcxo_4", "vctcxo", 1, 4, 0},
	{PXA1928_CLK_PLL1_2, "pll1_2", "pll1_624", 1, 2, 0},
	{PXA1928_CLK_PLL1_9, "pll1_9", "pll1_624", 1, 9, 0},
	{PXA1928_CLK_PLL1_12, "pll1_12", "pll1_624", 1, 12, 0},
	{PXA1928_CLK_PLL1_16, "pll1_16", "pll1_624", 1, 16, 0},
	{PXA1928_CLK_PLL1_20, "pll1_20", "pll1_624", 1, 20, 0},
};

/* pll2 ~ pll7 */
static struct pxa1928_clk_pll_vco_table pllx_vco_table[] = {
	/* input  out  out_offsted refdiv fbdiv icp kvco ssc_en offset_en*/
	{26000000, 1594666666, 1594666666, 3, 46, 3, 0xa, 0, 0},/*pll2*/
	{26000000, 1768000000, 1768000000, 3, 51, 3, 0xb, 0, 0},/*pll3*/
	{26000000, 2114666666, 2133333333, 3, 61, 3, 0xc, 0, 0},/*pll4*/
	{26000000, 1594666666, 1594666666, 3, 46, 3, 0xa, 0, 0},/*pll5*/
	{26000000, 2114666666, 2133333333, 3, 61, 3, 0xc, 0, 0},/*pll6*/
	{26000000, 2114666666, 2133333333, 3, 61, 3, 0xc, 0, 0},/*pll7*/
};

static struct pxa1928_clk_pll_out_table pllx_out_table[] = {
	/* input_rate output_rate div_sel */
	{1594666666, 797333333, 1},
	{1768000000, 884000000, 1},
	{2114666666, 1057333333, 1},
	{1594666666, 797333333, 1},
	{2114666666, 528666666, 2},
	{2114666666, 528666666, 2},
};

static struct pxa1928_clk_pll_out_table pllx_outp_table[] = {
	/* input_rate output_rate div_sel */
	{1594666666, 531555555, 8},
	{1768000000, 442000000, 2},
	{2114666666, 528666666, 2},
	{1594666666, 531555555, 8},
	{2114666666, 1057333333, 1},
	{2114666666, 1057333333, 1},
};

static struct pll_tbl pllx_tbl[] = {
	{
		.vco_name = "pll2_vco",
		.out_name = "pll2",
		.outp_name = "pll2p",
		.out_flags = 0,
		.outp_flags = PXA1928_PLL_USE_DIV_3 |
			PXA1928_PLL_USE_ENABLE_BIT,
		.vco_flags = PXA1928_PLL_POST_ENABLE,
		.vco_params = {
			1200000000, 3200000000UL, MPMU_PLL2CR, MPMU_PLL2_CTRL1,
			MPMU_PLL2_CTRL2, MPMU_PLL2_CTRL3, MPMU_PLL2_CTRL4,
			MPMU_POSR, POSR_PLL2_LOCK, APMU_AP_PLL_CTRL, 2,
			GATE_CTRL_SHIFT_PLL2, APMU_GLB_CLK_CTRL
		},
		.out_params = {
			1200000000, MPMU_PLL2_CTRL1, 3, 26, MPMU_PLL2_CTRL1
		},
		.outp_params = {
			1200000000, MPMU_PLL2_CTRL4, 3, 5, MPMU_PLL2_CTRL1
		}
	},
	{
		.vco_name = "pll3_vco",
		.out_name = "pll3",
		.outp_name = "pll3p",
		.out_flags = 0,
		.outp_flags = PXA1928_PLL_USE_ENABLE_BIT,
		.vco_params = {
			1200000000, 3200000000UL, MPMU_PLL3CR, MPMU_PLL3_CTRL1,
			MPMU_PLL3_CTRL2, MPMU_PLL3_CTRL3, MPMU_PLL3_CTRL4,
			MPMU_POSR2, POSR_PLL3_LOCK, APMU_AP_PLL_CTRL, 2,
			GATE_CTRL_SHIFT_PLL3
		},
		.out_params = {
			1200000000, MPMU_PLL3_CTRL1, 3, 26, MPMU_PLL3_CTRL1
		},
		.outp_params = {
			1200000000, MPMU_PLL3_CTRL4, 3, 5, MPMU_PLL3_CTRL1
		}
	},
	{
		.vco_name = "pll4_vco",
		.out_name = "pll4",
		.outp_name = "pll4p",
		.out_flags = PXA1928_PLL_USE_SYNC_DDR,
		.outp_flags = PXA1928_PLL_USE_ENABLE_BIT,
		.vco_params = {
			1200000000, 3200000000UL, MPMU_PLL4CR, MPMU_PLL4_CTRL1,
			MPMU_PLL4_CTRL2, MPMU_PLL4_CTRL3, MPMU_PLL4_CTRL4,
			MPMU_POSR2, POSR_PLL4_LOCK, APMU_AP_PLL_CTRL, 2,
			GATE_CTRL_SHIFT_PLL4
		},
		.out_params = {
			1200000000, APMU_MC_CLK_RES_CTRL, 3, 12, MPMU_PLL4_CTRL1
		},
		.outp_params = {
			1200000000, MPMU_PLL4_CTRL4, 3, 5, MPMU_PLL4_CTRL1
		}
	},
	{
		.vco_name = "pll5_vco",
		.out_name = "pll5",
		.outp_name = "pll5p",
		.out_flags = 0,
		.outp_flags = PXA1928_PLL_USE_DIV_3 |
				PXA1928_PLL_USE_ENABLE_BIT,
		.vco_params = {
			1200000000, 3200000000UL, MPMU_PLL5CR, MPMU_PLL5_CTRL1,
			MPMU_PLL5_CTRL2, MPMU_PLL5_CTRL3, MPMU_PLL5_CTRL4,
			MPMU_POSR2, POSR_PLL5_LOCK, APMU_AP_PLL_CTRL, 2,
			GATE_CTRL_SHIFT_PLL5
		},
		.out_params = {
			1200000000, MPMU_PLL5_CTRL1, 3, 26, MPMU_PLL5_CTRL1
		},
		.outp_params = {
			1200000000, MPMU_PLL5_CTRL4, 3, 5, MPMU_PLL5_CTRL1
		}
	},
	{
		.vco_name = "pll6_vco",
		.out_name = "pll6",
		.outp_name = "pll6p",
		.out_flags = 0,
		.outp_flags = PXA1928_PLL_USE_ENABLE_BIT,
		.vco_params = {
			1200000000, 3200000000UL, MPMU_PLL6CR, MPMU_PLL6_CTRL1,
			MPMU_PLL6_CTRL2, MPMU_PLL6_CTRL3, MPMU_PLL6_CTRL4,
			MPMU_POSR3, POSR_PLL6_LOCK, APMU_AP_PLL_CTRL, 2,
			GATE_CTRL_SHIFT_PLL6
		},
		.out_params = {
			1200000000, MPMU_PLL6_CTRL1, 3, 26, MPMU_PLL6_CTRL1
		},
		.outp_params = {
			1200000000, MPMU_PLL6_CTRL4, 3, 5, MPMU_PLL6_CTRL1
		}
	},
	{
		.vco_name = "pll7_vco",
		.out_name = "pll7",
		.outp_name = "pll7p",
		.out_flags = 0,
		.outp_flags = PXA1928_PLL_USE_ENABLE_BIT,
		.vco_params = {
			1200000000, 3200000000UL, MPMU_PLL7CR, MPMU_PLL7_CTRL1,
			MPMU_PLL7_CTRL2, MPMU_PLL7_CTRL3, MPMU_PLL7_CTRL4,
			MPMU_POSR2, POSR_PLL7_LOCK, APMU_AP_PLL_CTRL, 2,
			GATE_CTRL_SHIFT_PLL7
		},
		.out_params = {
			1200000000, MPMU_PLL7_CTRL1, 3, 26, MPMU_PLL7_CTRL1
		},
		.outp_params = {
			1200000000, MPMU_PLL7_CTRL4, 3, 5, MPMU_PLL7_CTRL1
		}
	}
};

static struct mmp_clk_factor_masks uart_factor_masks = {
	.factor = 2,
	.num_mask = 0x1fff,
	.den_mask = 0x1fff,
	.num_shift = 16,
	.den_shift = 0,
};

static struct mmp_clk_factor_tbl uart_factor_tbl[] = {
	{.num = 832, .den = 234},     /*14.745MHZ */
	{.num = 1, .den = 1},
};

static void pxa1928_pll_init(struct pxa1928_clk_unit *pxa_unit)
{
	struct clk *clk;
	struct mmp_clk_unit *unit = &pxa_unit->unit;
	u32 i, pll_num = ARRAY_SIZE(pllx_tbl);

	mmp_register_fixed_rate_clks(unit, fixed_rate_clks,
					ARRAY_SIZE(fixed_rate_clks));

	mmp_register_fixed_factor_clks(unit, fixed_factor_clks,
					ARRAY_SIZE(fixed_factor_clks));

	clk = mmp_clk_register_factor("uart_pll", "pll1_4",
				CLK_SET_RATE_PARENT,
				pxa_unit->mpmu_base + MPMU_UART_PLL,
				&uart_factor_masks, uart_factor_tbl,
				ARRAY_SIZE(uart_factor_tbl), NULL);
	mmp_clk_add(unit, PXA1928_CLK_UART_PLL, clk);

	/* PLL2~PLL7 */
	for (i = 0; i < pll_num; i++) {
		spin_lock_init(&pllx_tbl[i].lock);
		pllx_tbl[i].vco_tbl = &pllx_vco_table[i];
		pllx_tbl[i].out_tbl = &pllx_out_table[i];
		pllx_tbl[i].outp_tbl = &pllx_outp_table[i];
		clk = pxa1928_clk_register_pll_vco(pllx_tbl[i].vco_name,
			"vctcxo", 0, pllx_tbl[i].vco_flags,
			&pllx_tbl[i].lock, pxa_unit->mpmu_base, pxa_unit->apmu_base,
			&pllx_tbl[i].vco_params, pllx_tbl[i].vco_tbl);
		clk_register_clkdev(clk, pllx_tbl[i].vco_name, NULL);
		clk_set_rate(clk, pllx_tbl[i].vco_tbl[0].output_rate);

		clk = pxa1928_clk_register_pll_out(pllx_tbl[i].out_name,
			pllx_tbl[i].vco_name, pllx_tbl[i].out_flags,
			&pllx_tbl[i].lock, pxa_unit->mpmu_base, pxa_unit->apmu_base,
			&pllx_tbl[i].out_params, pllx_tbl[i].out_tbl);
		clk_register_clkdev(clk, pllx_tbl[i].out_name, NULL);
		clk_set_rate(clk, pllx_tbl[i].out_tbl[0].output_rate);

		clk = pxa1928_clk_register_pll_out(pllx_tbl[i].outp_name,
			pllx_tbl[i].vco_name, pllx_tbl[i].outp_flags,
			&pllx_tbl[i].lock, pxa_unit->mpmu_base, pxa_unit->apmu_base,
			&pllx_tbl[i].outp_params, pllx_tbl[i].outp_tbl);
		clk_register_clkdev(clk, pllx_tbl[i].outp_name, NULL);
		clk_set_rate(clk, pllx_tbl[i].outp_tbl[0].output_rate);
	}
}

static void pxa1928_acpu_init(struct pxa1928_clk_unit *pxa_unit)
{
	void __iomem *ddrdfc_base;

	ddrdfc_base = ioremap(0xd4282600, SZ_512);
	if (ddrdfc_base == NULL) {
		pr_err("error to ioremap ddfdfc base\n");
		return;
	}

	pxa1928_core_clk_init(pxa_unit->apmu_base);
	pxa1928_axi_clk_init(pxa_unit->apmu_base);
	pxa1928_axi11_clk_init(pxa_unit->apmu_base);
	pxa1928_ddr_clk_init(pxa_unit->apmu_base, ddrdfc_base,
		pxa_unit->ciu_base, &pxa_unit->unit);
}

static struct mmp_param_gate_clk apbc_gate_clks[] = {
	{PXA1928_CLK_TWSI0, "twsi0_clk", "vctcxo", CLK_SET_RATE_PARENT, APBC_TWSI0, 0x7, 0x3, 0x0, 0, NULL},
	{PXA1928_CLK_TWSI1, "twsi1_clk", "vctcxo", CLK_SET_RATE_PARENT, APBC_TWSI1, 0x7, 0x3, 0x0, 0, NULL},
	{PXA1928_CLK_TWSI2, "twsi2_clk", "vctcxo", CLK_SET_RATE_PARENT, APBC_TWSI2, 0x7, 0x3, 0x0, 0, NULL},
	{PXA1928_CLK_TWSI3, "twsi3_clk", "vctcxo", CLK_SET_RATE_PARENT, APBC_TWSI3, 0x7, 0x3, 0x0, 0, NULL},
	{PXA1928_CLK_TWSI4, "twsi4_clk", "vctcxo", CLK_SET_RATE_PARENT, APBC_TWSI4, 0x7, 0x3, 0x0, 0, NULL},
	{PXA1928_CLK_TWSI5, "twsi5_clk", "vctcxo", CLK_SET_RATE_PARENT, APBC_TWSI5, 0x7, 0x3, 0x0, 0, NULL},
	{PXA1928_CLK_GPIO, "gpio_clk", "vctcxo", CLK_SET_RATE_PARENT, APBC_GPIO, 0x7, 0x3, 0x0, 0, NULL},
	{PXA1928_CLK_KPC, "kpc_clk", "clk32", CLK_SET_RATE_PARENT, APBC_KPC, 0x7, 0x3, 0x0, MMP_CLK_GATE_NEED_DELAY, NULL},
	{PXA1928_CLK_RTC, "rtc_clk", "clk32", CLK_SET_RATE_PARENT, APBC_RTC, 0x7, 0x3, 0x0, MMP_CLK_GATE_NEED_DELAY, NULL},
	{PXA1928_CLK_PWM0, "pwm0_clk", "vctcxo", CLK_SET_RATE_PARENT, APBC_PWM0, 0x7, 0x3, 0x0, 0, NULL},
	{PXA1928_CLK_PWM1, "pwm1_clk", "vctcxo", CLK_SET_RATE_PARENT, APBC_PWM1, 0x7, 0x3, 0x0, 0, NULL},
	{PXA1928_CLK_PWM2, "pwm2_clk", "vctcxo", CLK_SET_RATE_PARENT, APBC_PWM2, 0x7, 0x3, 0x0, 0, NULL},
	{PXA1928_CLK_PWM3, "pwm3_clk", "vctcxo", CLK_SET_RATE_PARENT, APBC_PWM3, 0x7, 0x3, 0x0, 0, NULL},
};

static DEFINE_SPINLOCK(uart0_lock);
static DEFINE_SPINLOCK(uart1_lock);
static DEFINE_SPINLOCK(uart2_lock);
static DEFINE_SPINLOCK(uart3_lock);
static const char *uart_parent_names[] = {"uart_pll", "vctcxo"};

#ifdef CONFIG_CORESIGHT_SUPPORT
static void pxa1928_coresight_clk_init(struct pxa1928_clk_unit *pxa_unit)
{
	struct mmp_clk_unit *unit = &pxa_unit->unit;
	struct clk *clk;

	clk = mmp_clk_register_gate(NULL, "DBGCLK", "pll1_624", 0,
				pxa_unit->apmu_base + APMU_APDBG,
				0x10, 0x10, 0x0, 0, NULL);
	mmp_clk_add(unit, PXA1928_CLK_DBGCLK, clk);

	/* TMC clock */
	clk = mmp_clk_register_gate(NULL, "TRACECLK", "DBGCLK", 0,
				pxa_unit->apmu_base + APMU_AP_DEBUG1,
				(1 << 25), (1 << 25), 0x0, 0, NULL);
	mmp_clk_add(unit, PXA1928_CLK_TRACECLK, clk);
}
#endif

static void pxa1928_apb_periph_clk_init(struct pxa1928_clk_unit *pxa_unit)
{
	struct clk *clk;
	struct mmp_clk_unit *unit = &pxa_unit->unit;

	mmp_register_gate_clks(unit, apbc_gate_clks, pxa_unit->apbc_base,
				ARRAY_SIZE(apbc_gate_clks));

	clk = clk_register_mux(NULL, "uart0_mux", uart_parent_names,
				ARRAY_SIZE(uart_parent_names),
				CLK_SET_RATE_PARENT,
				pxa_unit->apbc_base + APBC_UART0,
				4, 1, 0, &uart0_lock);
	clk = mmp_clk_register_gate(NULL, "uart0_clk", "uart0_mux",
				CLK_SET_RATE_PARENT,
				pxa_unit->apbc_base + APBC_UART0,
				0x7, 0x3, 0x0, 0, &uart0_lock);
	mmp_clk_add(unit, PXA1928_CLK_UART0, clk);

	clk = clk_register_mux(NULL, "uart1_mux", uart_parent_names,
				ARRAY_SIZE(uart_parent_names),
				CLK_SET_RATE_PARENT,
				pxa_unit->apbc_base + APBC_UART1,
				4, 1, 0, &uart1_lock);
	clk = mmp_clk_register_gate(NULL, "uart1_clk", "uart1_mux",
				CLK_SET_RATE_PARENT,
				pxa_unit->apbc_base + APBC_UART0,
				0x7, 0x3, 0x0, 0, &uart1_lock);
	mmp_clk_add(unit, PXA1928_CLK_UART1, clk);

	clk = clk_register_mux(NULL, "uart2_mux", uart_parent_names,
				ARRAY_SIZE(uart_parent_names),
				CLK_SET_RATE_PARENT,
				pxa_unit->apbc_base + APBC_UART2,
				4, 1, 0, &uart2_lock);
	clk = mmp_clk_register_gate(NULL, "uart2_clk", "uart2_mux",
				CLK_SET_RATE_PARENT,
				pxa_unit->apbc_base + APBC_UART2,
				0x7, 0x3, 0x0, 0, &uart2_lock);
	mmp_clk_add(unit, PXA1928_CLK_UART2, clk);

	clk = clk_register_mux(NULL, "uart3_mux", uart_parent_names,
				ARRAY_SIZE(uart_parent_names),
				CLK_SET_RATE_PARENT,
				pxa_unit->apbc_base + APBC_UART3,
				4, 1, 0, &uart2_lock);
	clk = mmp_clk_register_gate(NULL, "uart3_clk", "uart3_mux",
				CLK_SET_RATE_PARENT,
				pxa_unit->apbc_base + APBC_UART3,
				0x7, 0x3, 0x0, 0, &uart3_lock);
	mmp_clk_add(unit, PXA1928_CLK_UART3, clk);

#ifdef CONFIG_CORESIGHT_SUPPORT
	pxa1928_coresight_clk_init(pxa_unit);
#endif
}

static DEFINE_SPINLOCK(sdh0_lock);
static const char *sdh_parent_names[] = {"pll1_624", "pll5p", "pll5", "pll1_416"};
static struct mmp_clk_mix_config sdh_mix_config = {
	.reg_info = DEFINE_MIX_REG_INFO(4, 10, 2, 8, 32),
};

static DEFINE_SPINLOCK(disp_lock);
static const char *disp1_parent_names[] = {"pll1_624", "pll1_416"};
static const char *vdma_parent_names[] = {"pll1_2", "pll1_416", "pll1_624"};

static const char *disp_axi_parent_names[] = {"pll1_2", "pll1_416", "pll1_624"};
static int disp_axi_mux_table[] = {0x0, 0x1, 0x2};
static struct mmp_clk_mix_config disp_axi_mix_config = {
	.reg_info = DEFINE_MIX_REG_INFO(3, 0, 3, 4, 32),
	.mux_table = disp_axi_mux_table,
	.div_flags = CLK_DIVIDER_ONE_BASED,
};

static void pxa1928_axi_periph_clk_init(struct pxa1928_clk_unit *pxa_unit)
{
	struct clk *clk;
	struct mmp_clk_unit *unit = &pxa_unit->unit;

	clk = mmp_clk_register_gate(NULL, "usb_clk", "usb_pll", 0,
				pxa_unit->apmu_base + APMU_USB,
				0x9, 0x9, 0x1, 0, NULL);
	mmp_clk_add(unit, PXA1928_CLK_USB, clk);

	sdh_mix_config.reg_info.reg_clk_ctrl = pxa_unit->apmu_base + APMU_SDH0;
	clk = mmp_clk_register_mix(NULL, "sdh_mix_clk", sdh_parent_names,
				ARRAY_SIZE(sdh_parent_names),
				CLK_SET_RATE_PARENT,
				&sdh_mix_config, &sdh0_lock);

	clk = mmp_clk_register_gate(NULL, "sdh0_clk", "sdh_mix_clk",
				CLK_SET_RATE_PARENT,
				pxa_unit->apmu_base + APMU_SDH0,
				0x1b, 0x1b, 0x0, 0, &sdh0_lock);
	mmp_clk_add(unit, PXA1928_CLK_SDH0, clk);

	clk = mmp_clk_register_gate(NULL, "sdh1_clk", "sdh_mix_clk",
				CLK_SET_RATE_PARENT,
				pxa_unit->apmu_base + APMU_SDH1,
				0x1b, 0x1b, 0x0, 0, NULL);
	mmp_clk_add(unit, PXA1928_CLK_SDH1, clk);

	clk = mmp_clk_register_gate(NULL, "sdh2_clk", "sdh_mix_clk",
				CLK_SET_RATE_PARENT,
				pxa_unit->apmu_base + APMU_SDH2,
				0x1b, 0x1b, 0x0, 0, NULL);
	mmp_clk_add(unit, PXA1928_CLK_SDH2, clk);

	clk = clk_register_mux(NULL, "disp1_sel_clk", disp1_parent_names,
				ARRAY_SIZE(disp1_parent_names),
				CLK_SET_RATE_PARENT,
				pxa_unit->apmu_base + APMU_DISP_CLK_CTRL,
				12, 3, 0, &disp_lock);
	mmp_clk_add(unit, PXA1928_CLK_DISP_DISP1_CLK, clk);

	clk = clk_register_mux(NULL, "vdma_sel_clk", vdma_parent_names,
				ARRAY_SIZE(vdma_parent_names),
				CLK_SET_RATE_PARENT,
				pxa_unit->apmu_base + APMU_DISP_CLK_CTRL,
				28, 3, 0, &disp_lock);
	mmp_clk_add(unit, PXA1928_CLK_DISP_VDMA_CLK, clk);

	clk = mmp_clk_register_gate(NULL, "disp1_en_clk", "disp1_sel_clk",
				CLK_SET_RATE_PARENT,
				pxa_unit->apbc_base + APMU_DISP_CLK_CTRL,
				0x700, 0x100, 0x0, 0, &disp_lock);
	mmp_clk_add(unit, PXA1928_CLK_DISP_DISP1_EN, clk);

	clk = mmp_clk_register_gate(NULL, "esc_clk", NULL,
				CLK_IS_ROOT,
				pxa_unit->apbc_base + APMU_DISP_CLK_CTRL,
				0x70, 0x10, 0x0, 0, &disp_lock);
	mmp_clk_add(unit, PXA1928_CLK_DISP_ESC_CLK, clk);

	clk = mmp_clk_register_gate(NULL, "vdma_clk", "vdma_sel_clk",
				CLK_SET_RATE_PARENT,
				pxa_unit->apbc_base + APMU_DISP_CLK_CTRL,
				0x7000000, 0x1000000, 0x0, 0, &disp_lock);
	mmp_clk_add(unit, PXA1928_CLK_DISP_VDMA_EN, clk);

	disp_axi_mix_config.reg_info.reg_clk_ctrl = pxa_unit->apmu_base + APMU_DISP_CLK_CTRL2;
	clk = mmp_clk_register_mix(NULL, "disp_axi_clk", disp_axi_parent_names,
				ARRAY_SIZE(disp_axi_parent_names),
				CLK_SET_RATE_PARENT,
				&disp_axi_mix_config, &disp_lock);
	mmp_clk_add(unit, PXA1928_CLK_DISP_AXI_CLK, clk);
}

static void __init pxa1928_clk_init(struct device_node *np)
{
	struct pxa1928_clk_unit *pxa_unit;

	pxa_unit = kzalloc(sizeof(*pxa_unit), GFP_KERNEL);
	if (!pxa_unit) {
		pr_err("failed to allocate memory for pxa1928 clock unit\n");
		return;
	}

	pxa_unit->mpmu_base = of_iomap(np, 0);
	if (!pxa_unit->mpmu_base) {
		pr_err("failed to map mpmu registers\n");
		return;
	}

	pxa_unit->apmu_base = of_iomap(np, 1);
	if (!pxa_unit->mpmu_base) {
		pr_err("failed to map apmu registers\n");
		return;
	}

	pxa_unit->apbc_base = of_iomap(np, 2);
	if (!pxa_unit->apbc_base) {
		pr_err("failed to map apbc registers\n");
		return;
	}

	pxa_unit->ciu_base = of_iomap(np, 3);
	if (!pxa_unit->ciu_base) {
		pr_err("failed to map ciu registers\n");
		return;
	}

	mmp_clk_init(np, &pxa_unit->unit, PXA1928_NR_CLKS);

	pxa1928_pll_init(pxa_unit);

	pxa1928_acpu_init(pxa_unit);

	pxa1928_apb_periph_clk_init(pxa_unit);

	pxa1928_axi_periph_clk_init(pxa_unit);
}

CLK_OF_DECLARE(pxa1928_clk, "marvell,pxa1928-clock", pxa1928_clk_init);