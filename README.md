
# Projet Hacheur MCC – Séance 1  
### ENSEA – 2526-S9-ESE_3 – Actionneurs & Automatique Appliquée  
### Projet Onduleur Triphasé Didactique – 60V / 10A  
### Rédaction : Houssam Hakki & Joao Pedro Penelu  

---

# Objectifs de la séance 1 – Commande MCC basique

- Générer 4 PWM complémentaires décalées sur TIM1. 
- Intégrer un temps mort (deadtime) sécurisé.
- Vérifier les signaux à l’oscilloscope.
- Effectuer un premier test moteur en boucle ouverte.
- Implémenter la commande `speed XXXX` via le shell UART.

---

# 1. Configuration PWM sur TIM1

Les PWM sont générées via TIM1 :

| Signal | Rôle | Type |
|-------|------|------|
| TIM1_CH1 | Phase U – High side | PWM |
| TIM1_CH1N | Phase U – High side | PWM |
| TIM1_CH2 | Phase V – Low side | PWM complémentaire |
| TIM1_CH2N | Phase V – Low side | PWM complémentaire |

![Figure 00](Photos/ioc.png)

---

## 1.1 Calcul du ARR pour 20 kHz

Clock timer ≈ 170 MHz

Période d’horloge du timer :

T = 1 / (170 MHz) ≈ 5.88 ns

Calcul de la valeur d’auto-rechargement (ARR) pour une fréquence PWM de 20 kHz :

ARR = 170 000 000 / 20 000 ≈ 8 500

→ Résolution ≈ 13 bits 
→ Compatible avec le cahier des charges (>10 bits)

Dans notre configuration, CubeMX génère **ARR = 4249**, ce qui correspond à ~20 kHz.

---

## 1.2 Calcul du deadtime

Temps mort souhaité : **200 ns**

deadtime (ticks) = 200 ns / 5.88 ns ≈ 35 ticks

→ Deadtime programmé : **DTG = 35**

---

## 1.3 Mode Center-Aligned

Nous utilisons le mode UpCounter pour générer nos PWM:

- Ce mode produit des signaux en comptage progressif (0 → ARR) avec des fronts alignés en début de période.
- Le cycle commence toujours lorsque le compteur repart à 0.
- Le front actif de la PWM apparaît lorsque le compteur atteint la valeur définie dans le registre CCR.
- La largeur d’impulsion (rapport cyclique) dépend directement de la valeur CCR, mais la position de l’impulsion reste toujours alignée en début de période.
- Le fonctionnement est simple, stable et adapté pour générer des PWM classiques.

---

# 2. Vérification Oscilloscope

Points vérifiés :

- Complémentarité CH1 / CH1N. 
- Deadtime correctement mesuré (~100 ns).
- Fréquence ≈ 20 kHz.
- Aucun recouvrement entre transistors haut/bas.

Les signaux observés confirment que la configuration du timer est correcte.
![Figure 01](Photos/ioc.png)
![Figure 02](Photos/ioc.png)
![Figure 03](Photos/ioc.png)

---

# 3. Implémentation de la commande Speed

Commande utilisée via le shell UART :

### Code dans `shell.c` :

```c
static int sh_speed(h_shell_t* h_shell, int argc, char** argv)
{
    uint32_t value = 0;
    uint32_t max = __HAL_TIM_GET_AUTORELOAD(&htim1); // Timer's ARR (maximum duty)
    int size;

    // 1. Check if an argument was given

    if (argc < 2) {
        size = snprintf(h_shell->print_buffer, SHELL_PRINT_BUFFER_SIZE,
                        "Usage: speed <0-%lu>\r\n", (unsigned long)max);
        h_shell->drv.transmit(h_shell->print_buffer, size);
        return -1;
    }

    // 2. Manual conversion of the characters to an integer

    char* p = argv[1];
    while (*p != '\0') {

        // Reject any non-digit character
        if (*p < '0' || *p > '9') {
            size = snprintf(h_shell->print_buffer, SHELL_PRINT_BUFFER_SIZE,
                            "Error: \"%s\" is not a valid number\r\n", argv[1]);
            h_shell->drv.transmit(h_shell->print_buffer, size);
            return -1;
        }

        // Build integer digit by digit
        value = value * 10 + (uint32_t)(*p - '0');
        p++;
    }

```
![Figure 04](Photos/ioc.png)

---
# 4. Tests du moteur

Après la génération et la vérification des PWM, nous avons procédé aux premiers tests du moteur à courant continu en boucle ouverte.
L'objectif était d'observer le comportement du moteur pour différents rapports cycliques afin d’identifier d’éventuels problèmes liés à l'appel de courant, à l'inertie ou au hacheur.

---

## Test 1 — Rapport cyclique 50 %

À 70 %, le moteur démarre et tourne de manière stable :

- Rotation régulière
- Vibrations faibles ou inexistantes
- Consommation raisonnable
- Aucun échauffement anormal

 Ce fonctionnement constitue une base saine pour les tests suivants.

---

## Test 2 — Rapport cyclique 70 %

Lors d’un passage direct de 50 % à 70 %, un phénomène indésirable apparaît :

- Montée brutale de courant
- Vibra­tions perceptibles
- Accélération instantanée trop rapide
- Risque pour la carte de puissance et pour le moteur

Ce comportement est **normal** en boucle ouverte :
le moteur n’a aucune rampe d'accélération, et l’augmentation soudaine du rapport cyclique provoque un **appel de courant important**, susceptible d’endommager les MOSFET ou la carte.

[![Video Demo](Photos/video_thumbnail.png)](Videos/demo.mp4)

---

# 5. Mise en place d’une rampe d’accélération (solution)

Pour éviter les appels de courant lors des changements rapides de rapport cyclique, nous avons implémenté une **rampe d’accélération**, conformément aux recommandations du sujet.

Cette rampe ajuste progressivement le rapport cyclique vers la valeur cible, avec un incrément contrôlé.

### Fonction implémentée : `motor_set_ramp()`

```c
void motor_set_ramp(uint16_t target)
{
    uint16_t current = motor_duty;

    while (current != target)
    {
        if (current < target) current++;
        else current--;

        motor_set_duty(current);
        HAL_Delay(1); // Durée ajustable pour contrôler la vitesse de la rampe
    }
}



