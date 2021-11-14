#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/gpio.h>                 // Required for the GPIO functions
#include <linux/interrupt.h>            // Required for the IRQ code
#include <linux/unistd.h>

MODULE_LICENSE("GPL");

static unsigned int gpioLED1 = 16;       ///< hard coding the LED gpio for this example to P9_23 (GPIO49)
static unsigned int gpioLED2 = 20;
static unsigned int gpioButtonA1 = 26;   ///< hard coding the button gpio for this example to P9_27 (GPIO115)
static unsigned int gpioButtonA2 = 19;
static unsigned int gpioButtonB1 = 13;
static unsigned int gpioButtonB2 = 21;
static unsigned int irqNumberA1;          ///< Used to share the IRQ number within this file
static unsigned int irqNumberA2;
static unsigned int irqNumberB1;
static unsigned int irqNumberB2;
static unsigned int numberPressesA1 = 0;  ///< For information, store the number of button presses
static unsigned int numberPressesA2 = 0;
static unsigned int numberPressesB1 = 0;
static unsigned int numberPressesB2 = 0;
static bool ledOn1 = 0;          ///< Is the LED on or off? Used to invert its state (off by default)
static bool	ledOn2 = 0;


/// Function prototype for the custom IRQ handler function -- see below for the implementation
static irq_handler_t ebbgpio_irq1_handler(unsigned int irq, void *dev_id, struct pt_regs *regs);
static irq_handler_t ebbgpio_irq2_handler(unsigned int irq, void *dev_id, struct pt_regs *regs);
static irq_handler_t ebbgpio_irq3_handler(unsigned int irq, void *dev_id, struct pt_regs *regs);
static irq_handler_t ebbgpio_irq4_handler(unsigned int irq, void *dev_id, struct pt_regs *regs);

/** @brief The LKM initialization function
 *  The static keyword restricts the visibility of the function to within this C file. The __init
 *  macro means that for a built-in driver (not a LKM) the function is only used at initialization
 *  time and that it can be discarded and its memory freed up after that point. In this example this
 *  function sets up the GPIOs and the IRQ
 *  @return returns 0 if successful
 */
static int __init ebbgpio_init(void){
   int result = 0;
   printk(KERN_INFO "GPIO_TEST: Initializing the GPIO_TEST LKM\n");
   // Is the GPIO a valid GPIO number (e.g., the BBB has 4x32 but not all available)
   if (!gpio_is_valid(gpioLED1)){
      printk(KERN_INFO "GPIO_TEST: invalid LED GPIO 1\n");
      return -ENODEV;
   }
   if (!gpio_is_valid(gpioLED2)){
      printk(KERN_INFO "GPIO_TEST: invalid LED GPIO 2\n");
      return -ENODEV;
   }

   // Going to set up the LED. It is a GPIO in output mode and will be on by default
   ledOn1 = false;
   ledOn2 = false;
   gpio_request(gpioLED1, "sysfs");          // gpioLED is hardcoded to 49, request it
   gpio_direction_output(gpioLED1, ledOn1);   // Set the gpio to be in output mode and on
// gpio_set_value(gpioLED, ledOn);          // Not required as set by line above (here for reference)
   gpio_export(gpioLED1, false);             // Causes gpio49 to appear in /sys/class/gpio
   											// the bool argument prevents the direction from being changed
   gpio_request(gpioLED2, "sysfs");
   gpio_direction_output(gpioLED2, ledOn2);
   gpio_export(gpioLED2, false);

   gpio_request(gpioButtonA1, "sysfs");       // Set up the gpioButton
   gpio_direction_input(gpioButtonA1);        // Set the button GPIO to be an input
   gpio_set_debounce(gpioButtonA1, 200);      // Debounce the button with a delay of 200ms
   gpio_export(gpioButtonA1, false);          // Causes gpio115 to appear in /sys/class/gpio

   gpio_request(gpioButtonA2, "sysfs");       // Set up the gpioButton
   gpio_direction_input(gpioButtonA2);        // Set the button GPIO to be an input
   gpio_set_debounce(gpioButtonA2, 200);      // Debounce the button with a delay of 200ms
   gpio_export(gpioButtonA2, false);

   gpio_request(gpioButtonB1, "sysfs");       // Set up the gpioButton
   gpio_direction_input(gpioButtonB1);        // Set the button GPIO to be an input
   gpio_set_debounce(gpioButtonB1, 200);      // Debounce the button with a delay of 200ms
   gpio_export(gpioButtonB1, false);

   gpio_request(gpioButtonB2, "sysfs");       // Set up the gpioButton
   gpio_direction_input(gpioButtonB2);        // Set the button GPIO to be an input
   gpio_set_debounce(gpioButtonB2, 200);      // Debounce the button with a delay of 200ms
   gpio_export(gpioButtonB2, false);


   printk(KERN_INFO "GPIO_TEST: The button A state is currently: %d\n", gpio_get_value(gpioButtonA1));
   printk(KERN_INFO "GPIO_TEST: The button B state is currently: %d\n", gpio_get_value(gpioButtonA2));
   printk(KERN_INFO "GPIO_TEST: The button C state is currently: %d\n", gpio_get_value(gpioButtonB1));
   printk(KERN_INFO "GPIO_TEST: The button D state is currently: %d\n", gpio_get_value(gpioButtonB2));


   // GPIO numbers and IRQ numbers are not the same! This function performs the mapping for us
   irqNumberA1 = gpio_to_irq(gpioButtonA1);
   printk(KERN_INFO "GPIO_TEST: The button A is mapped to IRQ: %d\n", irqNumberA1);
   irqNumberA2 = gpio_to_irq(gpioButtonA2);
   printk(KERN_INFO "GPIO_TEST: The button B is mapped to IRQ: %d\n", irqNumberA2);
   irqNumberB1 = gpio_to_irq(gpioButtonB1);
   printk(KERN_INFO "GPIO_TEST: The button C is mapped to IRQ: %d\n", irqNumberB1);
   irqNumberB2 = gpio_to_irq(gpioButtonB2);
   printk(KERN_INFO "GPIO_TEST: The button D is mapped to IRQ: %d\n", irqNumberB2);

   // This next call requests an interrupt line
   result = request_irq(irqNumberA1,             // The interrupt number requested
                        (irq_handler_t) ebbgpio_irq1_handler, // The pointer to the handler function below
                        IRQF_TRIGGER_RISING,   // Interrupt on rising edge (button press, not release)
                        "ebb_gpio_handler",    // Used in /proc/interrupts to identify the owner
                        NULL);                 // The *dev_id for shared interrupt lines, NULL is okay

   printk(KERN_INFO "GPIO_TEST: The interrupt request 1 result is: %d\n", result);

   result = request_irq(irqNumberA2,             // The interrupt number requested
                        (irq_handler_t) ebbgpio_irq2_handler, // The pointer to the handler function below
                        IRQF_TRIGGER_RISING,   // Interrupt on rising edge (button press, not release)
                        "ebb_gpio_handler",    // Used in /proc/interrupts to identify the owner
                        NULL);                 // The *dev_id for shared interrupt lines, NULL is okay

   printk(KERN_INFO "GPIO_TEST: The interrupt request 2 result is: %d\n", result);

   result = request_irq(irqNumberB1,             // The interrupt number requested
                        (irq_handler_t) ebbgpio_irq3_handler, // The pointer to the handler function below
                        IRQF_TRIGGER_RISING,   // Interrupt on rising edge (button press, not release)
                        "ebb_gpio_handler",    // Used in /proc/interrupts to identify the owner
                        NULL);                 // The *dev_id for shared interrupt lines, NULL is okay

   printk(KERN_INFO "GPIO_TEST: The interrupt request 3 result is: %d\n", result);

   result = request_irq(irqNumberB2,             // The interrupt number requested
                        (irq_handler_t) ebbgpio_irq4_handler, // The pointer to the handler function below
                        IRQF_TRIGGER_RISING,   // Interrupt on rising edge (button press, not release)
                        "ebb_gpio_handler",    // Used in /proc/interrupts to identify the owner
                        NULL);                 // The *dev_id for shared interrupt lines, NULL is okay

   printk(KERN_INFO "GPIO_TEST: The interrupt request 4 result is: %d\n", result);

   return result;
}

/** @brief The LKM cleanup function
 *  Similar to the initialization function, it is static. The __exit macro notifies that if this
 *  code is used for a built-in driver (not a LKM) that this function is not required. Used to release the
 *  GPIOs and display cleanup messages.
 */
static void __exit ebbgpio_exit(void){

   printk(KERN_INFO "GPIO_TEST: The button state A is currently: %d\n", gpio_get_value(gpioButtonA1));
   printk(KERN_INFO "GPIO_TEST: The button was pressed %d times\n", numberPressesA1);
   printk(KERN_INFO "GPIO_TEST: The button state B is currently: %d\n", gpio_get_value(gpioButtonA2));
   printk(KERN_INFO "GPIO_TEST: The button was pressed %d times\n", numberPressesA2);
   printk(KERN_INFO "GPIO_TEST: The button state C is currently: %d\n", gpio_get_value(gpioButtonB1));
   printk(KERN_INFO "GPIO_TEST: The button was pressed %d times\n", numberPressesB1);
   printk(KERN_INFO "GPIO_TEST: The button state D is currently: %d\n", gpio_get_value(gpioButtonB2));
   printk(KERN_INFO "GPIO_TEST: The button was pressed %d times\n", numberPressesB2);

   gpio_set_value(gpioLED1, 0);              // Turn the LED off, makes it clear the device was unloaded
   gpio_set_value(gpioLED2, 0);

   gpio_unexport(gpioLED1);                  // Unexport the LED GPIO
   gpio_unexport(gpioLED2);

   free_irq(irqNumberA1, NULL);               // Free the IRQ number, no *dev_id required in this case
   free_irq(irqNumberA2, NULL);
   free_irq(irqNumberB1, NULL);
   free_irq(irqNumberB2, NULL);

   gpio_unexport(gpioButtonA1);               // Unexport the Button GPIO
   gpio_unexport(gpioButtonA2);
   gpio_unexport(gpioButtonB1);
   gpio_unexport(gpioButtonB2);

   gpio_free(gpioLED1);                      // Free the LED GPIO
   gpio_free(gpioLED2);
   gpio_free(gpioButtonA1);                   // Free the Button GPIO
   gpio_free(gpioButtonA2);
   gpio_free(gpioButtonB1);
   gpio_free(gpioButtonB2);
   printk(KERN_INFO "GPIO_TEST: Goodbye from the LKM!\n");
}

/** @brief The GPIO IRQ Handler function
 *  This function is a custom interrupt handler that is attached to the GPIO above. The same interrupt
 *  handler cannot be invoked concurrently as the interrupt line is masked out until the function is complete.
 *  This function is static as it should not be invoked directly from outside of this file.
 *  @param irq    the IRQ number that is associated with the GPIO -- useful for logging.
 *  @param dev_id the *dev_id that is provided -- can be used to identify which device caused the interrupt
 *  Not used in this example as NULL is passed.
 *  @param regs   h/w specific register values -- only really ever used for debugging.
 *  return returns IRQ_HANDLED if successful -- should return IRQ_NONE otherwise.
 */
static irq_handler_t ebbgpio_irq1_handler(unsigned int irq, void *dev_id, struct pt_regs *regs){
	char *argv[4];
	char *envp[4];

	argv[0] = "/home/pi/Documents/ASO/Practica";
	argv[1] = "-c";
	argv[2] = "./boto1.sh";
	argv[3] = NULL;

	envp[0] = "HOME=/";
	envp[1] = "TERM=linux";
	envp[2] = "PATH=/sbin:/usr/sbin:/bin:/usr/bin";
	envp[3] = NULL;

	call_usermodehelper(argv[0], argv, envp, UMH_WAIT_PROC);

	ledOn1 = true;                          // Invert the LED state on each button press
	gpio_set_value(gpioLED1, ledOn1);          // Set the physical LED accordingly
	printk(KERN_INFO "GPIO_TEST: Interrupt! (button state is %d)\n", gpio_get_value(gpioButtonA1));
	numberPressesA1++;                        // Global counter, will be outputted when the module is unloaded
	return (irq_handler_t) IRQ_HANDLED;      // Announce that the IRQ has been handled correctly
}

static irq_handler_t ebbgpio_irq2_handler(unsigned int irq, void *dev_id, struct pt_regs *regs){
	ledOn1 = false;                          // Invert the LED state on each button press
	gpio_set_value(gpioLED1, ledOn1);          // Set the physical LED accordingly
	printk(KERN_INFO "GPIO_TEST: Interrupt! (button state is %d)\n", gpio_get_value(gpioButtonA2));
	numberPressesA2++;                       // Global counter, will be outputted when the module is unloaded
	return (irq_handler_t) IRQ_HANDLED;      // Announce that the IRQ has been handled correctly
}

static irq_handler_t ebbgpio_irq3_handler(unsigned int irq, void *dev_id, struct pt_regs *regs){
   ledOn2 = true;                          // Invert the LED state on each button press
   gpio_set_value(gpioLED2, ledOn2);          // Set the physical LED accordingly
   printk(KERN_INFO "GPIO_TEST: Interrupt! (button state is %d)\n", gpio_get_value(gpioButtonB1));
   numberPressesB1++;                          // Global counter, will be outputted when the module is unloaded
   return (irq_handler_t) IRQ_HANDLED;      // Announce that the IRQ has been handled correctly
}

static irq_handler_t ebbgpio_irq4_handler(unsigned int irq, void *dev_id, struct pt_regs *regs){
   ledOn2 = false;                          // Invert the LED state on each button press
   gpio_set_value(gpioLED2, ledOn2);          // Set the physical LED accordingly
   printk(KERN_INFO "GPIO_TEST: Interrupt! (button state is %d)\n", gpio_get_value(gpioButtonB2));
   numberPressesB2++;                        // Global counter, will be outputted when the module is unloaded
   return (irq_handler_t) IRQ_HANDLED;      // Announce that the IRQ has been handled correctly
}

/// This next calls are  mandatory -- they identify the initialization function
/// and the cleanup function (as above).
module_init(ebbgpio_init);
module_exit(ebbgpio_exit);
