#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/unistd.h>

MODULE_LICENSE("GPL");

static unsigned int gpioLED1 = 16;
static unsigned int gpioLED2 = 20;
static unsigned int gpioButtonA1 = 26;
static unsigned int gpioButtonA2 = 19;
static unsigned int gpioButtonB1 = 13;
static unsigned int gpioButtonB2 = 21;
static unsigned int irqNumberA1;
static unsigned int irqNumberA2;
static unsigned int irqNumberB1;
static unsigned int irqNumberB2;
static unsigned int numberPressesA1 = 0;
static unsigned int numberPressesA2 = 0;
static unsigned int numberPressesB1 = 0;
static unsigned int numberPressesB2 = 0;
static bool ledOn1 = 0;
static bool	ledOn2 = 0;



static irq_handler_t ebbgpio_irq1_handler(unsigned int irq, void *dev_id, struct pt_regs *regs);
static irq_handler_t ebbgpio_irq2_handler(unsigned int irq, void *dev_id, struct pt_regs *regs);
static irq_handler_t ebbgpio_irq3_handler(unsigned int irq, void *dev_id, struct pt_regs *regs);
static irq_handler_t ebbgpio_irq4_handler(unsigned int irq, void *dev_id, struct pt_regs *regs);


static int __init ebbgpio_init(void){
	int result = 0;
	printk(KERN_INFO "GPIO_TEST: Initializing the GPIO_TEST LKM\n");

	if (!gpio_is_valid(gpioLED1)){
	  printk(KERN_INFO "GPIO_TEST: invalid LED GPIO 1\n");
	  return -ENODEV;
	}
	if (!gpio_is_valid(gpioLED2)){
	  printk(KERN_INFO "GPIO_TEST: invalid LED GPIO 2\n");
	  return -ENODEV;
	}

	//Valors inicials
	ledOn1 = false;
	ledOn2 = false;

	//Es demana el pin GPIO dels leds i botons, es configuren i s'exporten.
	gpio_request(gpioLED1, "sysfs");
	gpio_direction_output(gpioLED1, ledOn1);
	gpio_export(gpioLED1, false);

	gpio_request(gpioLED2, "sysfs");
	gpio_direction_output(gpioLED2, ledOn2);
	gpio_export(gpioLED2, false);

	gpio_request(gpioButtonA1, "sysfs");
	gpio_direction_input(gpioButtonA1);
	gpio_set_debounce(gpioButtonA1, 200);
	gpio_export(gpioButtonA1, false);

	gpio_request(gpioButtonA2, "sysfs");
	gpio_direction_input(gpioButtonA2);
	gpio_set_debounce(gpioButtonA2, 200);
	gpio_export(gpioButtonA2, false);

	gpio_request(gpioButtonB1, "sysfs");
	gpio_direction_input(gpioButtonB1);
	gpio_set_debounce(gpioButtonB1, 200);
	gpio_export(gpioButtonB1, false);

	gpio_request(gpioButtonB2, "sysfs");
	gpio_direction_input(gpioButtonB2);
	gpio_set_debounce(gpioButtonB2, 200);
	gpio_export(gpioButtonB2, false);


	printk(KERN_INFO "GPIO_TEST: The button A state is currently: %d\n", gpio_get_value(gpioButtonA1));
	printk(KERN_INFO "GPIO_TEST: The button B state is currently: %d\n", gpio_get_value(gpioButtonA2));
	printk(KERN_INFO "GPIO_TEST: The button C state is currently: %d\n", gpio_get_value(gpioButtonB1));
	printk(KERN_INFO "GPIO_TEST: The button D state is currently: %d\n", gpio_get_value(gpioButtonB2));


	//Asociació del numero GPIO al numero IRQ
	irqNumberA1 = gpio_to_irq(gpioButtonA1);
	printk(KERN_INFO "GPIO_TEST: The button A is mapped to IRQ: %d\n", irqNumberA1);
	irqNumberA2 = gpio_to_irq(gpioButtonA2);
	printk(KERN_INFO "GPIO_TEST: The button B is mapped to IRQ: %d\n", irqNumberA2);
	irqNumberB1 = gpio_to_irq(gpioButtonB1);
	printk(KERN_INFO "GPIO_TEST: The button C is mapped to IRQ: %d\n", irqNumberB1);
	irqNumberB2 = gpio_to_irq(gpioButtonB2);
	printk(KERN_INFO "GPIO_TEST: The button D is mapped to IRQ: %d\n", irqNumberB2);


	result = request_irq(irqNumberA1,             // Numero de la interrupcio
	                    (irq_handler_t) ebbgpio_irq1_handler, // Punter cap a la funció
	                    IRQF_TRIGGER_RISING,    // Interrupcio quan es detecta el boto, no al deixar anar
	                    "ebb_gpio_handler",    // Indentifica el proprietari
	                    NULL);

	printk(KERN_INFO "GPIO_TEST: The interrupt request 1 result is: %d\n", result);

	result = request_irq(irqNumberA2,             // Numero de la interrupcio
	                    (irq_handler_t) ebbgpio_irq2_handler, // Punter cap a la funció
	                    IRQF_TRIGGER_RISING,    // Interrupcio quan es detecta el boto, no al deixar anar
	                    "ebb_gpio_handler",    // Indentifica el proprietari
	                    NULL);

	printk(KERN_INFO "GPIO_TEST: The interrupt request 2 result is: %d\n", result);

	result = request_irq(irqNumberB1,             // Numero de la interrupcio
	                    (irq_handler_t) ebbgpio_irq3_handler, // Punter cap a la funció
	                    IRQF_TRIGGER_RISING,    // Interrupcio quan es detecta el boto, no al deixar anar
	                    "ebb_gpio_handler",    // Indentifica el proprietari
	                    NULL);

	printk(KERN_INFO "GPIO_TEST: The interrupt request 3 result is: %d\n", result);

	result = request_irq(irqNumberB2,             // Numero de la interrupcio
	                    (irq_handler_t) ebbgpio_irq4_handler, // Punter cap a la funció
	                    IRQF_TRIGGER_RISING,   // Interrupcio quan es detecta el boto, no al deixar anar
	                    "ebb_gpio_handler",    // Indentifica el proprietari
	                    NULL);

	printk(KERN_INFO "GPIO_TEST: The interrupt request 4 result is: %d\n", result);

	return result;
}

//Funció exit
static void __exit ebbgpio_exit(void){

	printk(KERN_INFO "GPIO_TEST: The button state A is currently: %d\n", gpio_get_value(gpioButtonA1));
	printk(KERN_INFO "GPIO_TEST: The button was pressed %d times\n", numberPressesA1);
	printk(KERN_INFO "GPIO_TEST: The button state B is currently: %d\n", gpio_get_value(gpioButtonA2));
	printk(KERN_INFO "GPIO_TEST: The button was pressed %d times\n", numberPressesA2);
	printk(KERN_INFO "GPIO_TEST: The button state C is currently: %d\n", gpio_get_value(gpioButtonB1));
	printk(KERN_INFO "GPIO_TEST: The button was pressed %d times\n", numberPressesB1);
	printk(KERN_INFO "GPIO_TEST: The button state D is currently: %d\n", gpio_get_value(gpioButtonB2));
	printk(KERN_INFO "GPIO_TEST: The button was pressed %d times\n", numberPressesB2);

	gpio_set_value(gpioLED1, 0);
	gpio_set_value(gpioLED2, 0);

	gpio_unexport(gpioLED1);
	gpio_unexport(gpioLED2);

	free_irq(irqNumberA1, NULL);
	free_irq(irqNumberA2, NULL);
	free_irq(irqNumberB1, NULL);
	free_irq(irqNumberB2, NULL);

	gpio_unexport(gpioButtonA1);
	gpio_unexport(gpioButtonA2);
	gpio_unexport(gpioButtonB1);
	gpio_unexport(gpioButtonB2);

	gpio_free(gpioLED1);
	gpio_free(gpioLED2);
	gpio_free(gpioButtonA1);
	gpio_free(gpioButtonA2);
	gpio_free(gpioButtonB1);
	gpio_free(gpioButtonB2);
	printk(KERN_INFO "GPIO_TEST: Goodbye from the LKM!\n");
}

//Handler del boto 1
static irq_handler_t ebbgpio_irq1_handler(unsigned int irq, void *dev_id, struct pt_regs *regs){
	/*char *argv[4];
	char *envp[4];

	argv[0] = "/home/pi/Documents/ASO/Practica";
	argv[1] = "-c";
	argv[2] = "./boto1.sh";
	argv[3] = NULL;

	envp[0] = "HOME=/";
	envp[1] = "TERM=linux";
	envp[2] = "PATH=/sbin:/usr/sbin:/bin:/usr/bin";
	envp[3] = NULL;

	call_usermodehelper(argv[0], argv, envp, UMH_WAIT_PROC);*/

	ledOn1 = true;
	gpio_set_value(gpioLED1, ledOn1);
	printk(KERN_INFO "GPIO_TEST: Interrupt! (button state is %d)\n", gpio_get_value(gpioButtonA1));
	numberPressesA1++;
	return (irq_handler_t) IRQ_HANDLED;
}

//Handler del boto 2
static irq_handler_t ebbgpio_irq2_handler(unsigned int irq, void *dev_id, struct pt_regs *regs){
	/*char *argv[4];
	char *envp[4];

	argv[0] = "/home/pi/Documents/ASO/Practica";
	argv[1] = "-c";
	argv[2] = "./boto2.sh";
	argv[3] = NULL;

	envp[0] = "HOME=/";
	envp[1] = "TERM=linux";
	envp[2] = "PATH=/sbin:/usr/sbin:/bin:/usr/bin";
	envp[3] = NULL;

	call_usermodehelper(argv[0], argv, envp, UMH_WAIT_PROC);*/


	ledOn1 = false;
	gpio_set_value(gpioLED1, ledOn1);
	printk(KERN_INFO "GPIO_TEST: Interrupt! (button state is %d)\n", gpio_get_value(gpioButtonA2));
	numberPressesA2++;
	return (irq_handler_t) IRQ_HANDLED;
}

//Handler del boto 3
static irq_handler_t ebbgpio_irq3_handler(unsigned int irq, void *dev_id, struct pt_regs *regs){
	/*char *argv[4];
	char *envp[4];

	argv[0] = "/home/pi/Documents/ASO/Practica";
	argv[1] = "-c";
	argv[2] = "./boto3.sh";
	argv[3] = NULL;

	envp[0] = "HOME=/";
	envp[1] = "TERM=linux";
	envp[2] = "PATH=/sbin:/usr/sbin:/bin:/usr/bin";
	envp[3] = NULL;

	call_usermodehelper(argv[0], argv, envp, UMH_WAIT_PROC);*/

	ledOn2 = true;
	gpio_set_value(gpioLED2, ledOn2);
	printk(KERN_INFO "GPIO_TEST: Interrupt! (button state is %d)\n", gpio_get_value(gpioButtonB1));
	numberPressesB1++;
	return (irq_handler_t) IRQ_HANDLED;
}

//Handler del boto 4
static irq_handler_t ebbgpio_irq4_handler(unsigned int irq, void *dev_id, struct pt_regs *regs){
	/*char *argv[4];
	char *envp[4];

	argv[0] = "/home/pi/Documents/ASO/Practica";
	argv[1] = "-c";
	argv[2] = "./boto4.sh";
	argv[3] = NULL;

	envp[0] = "HOME=/";
	envp[1] = "TERM=linux";
	envp[2] = "PATH=/sbin:/usr/sbin:/bin:/usr/bin";
	envp[3] = NULL;

	call_usermodehelper(argv[0], argv, envp, UMH_WAIT_PROC);*/

	ledOn2 = false;
	gpio_set_value(gpioLED2, ledOn2);
	printk(KERN_INFO "GPIO_TEST: Interrupt! (button state is %d)\n", gpio_get_value(gpioButtonB2));
	numberPressesB2++;
	return (irq_handler_t) IRQ_HANDLED;
}

module_init(ebbgpio_init);
module_exit(ebbgpio_exit);
