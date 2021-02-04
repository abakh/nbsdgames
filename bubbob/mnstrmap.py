
class Monster1:
    def __init__(self, x, y, dir, player=0):
        self.x = x
        self.y = y
        self.dir = (1, -1)[dir]
        self.player = player

def nrange(start,n):
    return range(start,start+n)

class Nasty(Monster1):
    right  = nrange(239,4)
    left   = nrange(243,4)
    jailed = nrange(247,3)
    dead   = nrange(253,4)
    right_angry = nrange(800,4)
    left_angry  = nrange(804,4)

class Monky(Monster1):
    right  = nrange(265,4)
    left   = nrange(269,4)
    jailed = nrange(273,3)
    dead   = nrange(279,4)
    left_weapon = right_weapon = nrange(451,1)  # FIXME
    decay_weapon = nrange(452,4)                # FIXME
    right_angry = nrange(808,4)
    left_angry  = nrange(812,4)

class Ghosty(Monster1):
    right  = nrange(291,4)
    left   = nrange(295,4)
    jailed = nrange(299,3)
    dead   = nrange(305,4)
    right_angry = nrange(816,4)
    left_angry  = nrange(820,4)

class Flappy(Monster1):
    right  = nrange(317,4)
    left   = nrange(321,4)
    jailed = nrange(325,3)
    dead   = nrange(331,4)
    right_angry = nrange(824,4)
    left_angry  = nrange(828,4)

class Springy(Monster1):
    right  = nrange(343,4)
    left   = nrange(347,4)
    jailed = nrange(351,3)
    dead   = nrange(357,4)
    right_jump = nrange(369,2)
    left_jump  = nrange(371,2)
    right_angry = nrange(832,4)
    left_angry  = nrange(836,4)
    right_jump_angry = nrange(840,2)
    left_jump_angry  = nrange(842,2)

class Orcy(Monster1):
    right  = nrange(373,4)
    left   = nrange(377,4)
    jailed = nrange(381,3)
    dead   = nrange(387,4)
    left_weapon  = nrange(456,4)
    right_weapon = nrange(460,4)
    decay_weapon = nrange(456,0)                # FIXME
    right_angry = nrange(844,4)
    left_angry  = nrange(848,4)

class Gramy(Monster1):
    right  = nrange(399,4)
    left   = nrange(403,4)
    jailed = nrange(407,3)
    dead   = nrange(413,4)
    left_weapon = right_weapon = nrange(472,4)  # FIXME
    right_angry = nrange(852,4)
    left_angry  = nrange(856,4)

class Blitzy(Monster1):
    right = left = nrange(425,4)
    jailed = nrange(429,3)
    dead   = nrange(435,4)
    right_angry = left_angry = nrange(860,4)
    left_weapon = right_weapon = nrange(476,1)  # FIXME

class Ghost:
    left  = nrange(443,4)
    right = nrange(447,4)

class PlayerBubbles:
    appearing = nrange(952,5)
    bubble    = nrange(910,3)
    explosion = nrange(913,3)
    left_weapon  = nrange(464,4)
    right_weapon = nrange(468,4)
    decay_weapon = []

class LetterBubbles:
    Extend = nrange(128,3)     # FIXME
    eXtend = nrange(136,3)
    exTend = nrange(144,3)
    extEnd = nrange(152,3)
    exteNd = nrange(160,3)
    extenD = nrange(168,3)

class DyingBubble:
    first  = nrange(163,3)
    medium = nrange(171,3)
    last   = nrange(155,3)

class Fire:
    ground = nrange(490,4)
    drop = 489

class Lightning:
    fired = 488

class Water:
    h_flow = 900
    v_flow = 901
    start_right = 902
    start_left  = 904
    bottom = 903
    top    = 905
    tl_corner = 906
    bl_corner = 907
    br_corner = 908
    tr_corner = 909

class Flood:
    waves = nrange(140,4)
    fill  = 495

class MiscPoints:
    pink_100 = 496

class DigitsMisc:
    digits_mask = nrange(519,10)
    digits_border = nrange(920,10)
    digits_white = nrange(930,10)

class PotionBonuses:
    coin    = 477
    flower  = 478
    trefle  = 479
    rainbow = 480
    green_note = 481
    blue_note = 692


class Bonuses:
    monster_bonuses = [
        (593,1000), # banana
        (594,2000), # peach
        (595,3000), # quince
        (596,4000), # pastec
        (597,5000), # wine
        (598,6000), # ananas
        (599,8000) # diamond
        ]

    door = 139 # Lots of diamonds

    red_potion    = 637 #\  .
    green_potion  = 638 # > Clean the level and fill the top 5 lines with one of the PotionBonuses.
    yellow_potion = 639 #/

    kirsh     = 600
    icecream1 = 601 # NOT_USED
    erdbeer   = 602
    fish1     = 603
    tomato    = 604
    donut     = 605
    apple     = 606
    corn      = 607
    icecream2 = 608 # NOT_USED
    radish    = 609

    cyan_ice   = 610 #\  .
    violet_ice = 611 #|
    peach2     = 612 # > Produced from the bubbles after a wand.
    pastec2    = 613 #|
    cream_pie  = 614 #|
    sugar_pie  = 615 #/

    brown_wand  = 620 #\  .
    yellow_wand = 621 #|
    green_wand  = 622 # > Bubbles turn into bonus of the previous set after
    violet_wand = 623 # > the death of the last enemy plus a mega-bonus.
    blue_wand   = 624 #|
    red_wand    = 625 #/

    violet_chest = 626 #\  .
    blue_chest   = 627 # > Bubbles turn into diamonds plus after the death
    red_chest    = 628 # > of the last enemy plus a mega-diamond
    yellow_chest = 629 #/

    shoe = 631            # speed player movements
    grenade = 632         # put fire everywhere

    brown_umbrella  = 633 # fire rain
    grey_umbrella   = 634 # water rain
    violet_umbrella = 635 # spinning balls rain

    clock = 636 # time travel
    coffee = 641 # Speed player's movements and fire rate.
    book = 642 # Produces stars the middle-top going in any direction which kill the enemy upon contact.
    heart_poison = 643 # Froze the enemy and they are now killed on contact.
    gold_crux = 644    # become a bubble
    red_crux  = 645    # become a monster
    blue_crux = 646    # become a monster
    extend = 647 # Give 100'000 Points to the player and finish the level.

    ring = 640             # lord of the ring
    green_pepper = 648     # hot stuff!
    orange_thing = 649     # slippy
    aubergine = 650        # rear gear
    carrot = 651           # angry monsters
    rape = 652             # auto-fire
    white_carrot = 653     # fly
    chickpea = 654         # shield
    mushroom = 655         # pinball mode
    egg = 656              # players permutation
    chestnut = 657         # variation of frames per second
    green_thing = 658      # sugar bomb
    icecream3 = 659        # \   each icecream becomes two of the
    icecream4 = 660        #  \  next kind, which scores more points
    icecream5 = 661        #  /  that's a lot of points in total
    icecream6 = 662        # /
    softice1 = 663         # shoot farther
    softice2 = 665         # shoot nearer1
    french_fries = 664     # shoot 10 lightning bubbles
    custard_pie = 666      # shoot faster
    lollipop = 667         # invert left and right
    cocktail = 668         # short-lived bubbles
    ham = 669              # wall builder
    bomb = 670             # explodes the structure of the level
    beer = 671             # shoot 10 water bubbles
    emerald = 672          # mega points
    fish2 = 673            # mega blitz
    sapphire = 681         # mega points
    ruby = 682             # mega points
    tin = 674              # angry (double-speed) player
    hamburger = 675        # shoot 10 fire bubbles
    insect = 676            # walls fall down
    blue_necklace   = 677   # player ubiquity
    violet_necklace = 679   # monster ubiquity
    butterfly = 678          # lunar gravity
    conch = 680              # complete water flood
    yellow_sugar = 630      # from a bonbon bomb
    blue_sugar   = 691      # from a bonbon bomb

class Diamonds:
    # Produced from the bubbles after last enemy is killed and a chest or wand has been caught.
    violet = 616
    blue   = 617
    red    = 618
    yellow = 619

class Stars:
    # Effect of the book. Kill monsters on contact.
    blue    = nrange(940,2)
    yellow  = nrange(942,2)
    red     = nrange(944,2)
    green   = nrange(946,2)
    magenta = nrange(948,2)
    cyan    = nrange(950,2)
    COLORS  = ['blue', 'yellow', 'red', 'green', 'magenta', 'cyan']

class SpinningBalls:
    free = nrange(482,4)
    bubbled = nrange(486,2) # NOT_USED

class BigImages:
    cyan_ice   = 10  # Megabonus produced after a wand
    violet_ice = 11
    peach2     = 12
    pastec2    = 13
    cream_pie  = 14
    sugar_pie  = 15
    violet     = 16
    blue       = 17
    red        = 18
    yellow     = 19
    blitz      = 30
    hurryup    = nrange(31,2)

class birange:
    def __init__(self, a,b,n):
        self.a = a
        self.n = n
    def __getitem__(self, pn):
        return range(self.a + 1000*pn, self.a + 1000*pn + self.n)

class bidict:
    def __init__(self, a,b):
        self.a = a.items()
    def __getitem__(self, pn):
        pn *= 1000
        d = {}
        for key, value in self.a:
            d[key] = value + pn
        return d

class GreenAndBlue:
    water_bubbles = birange(182,185,3)
    fire_bubbles = birange(176,554,3)
    light_bubbles = birange(179,557,3)
    normal_bubbles = birange(188,195,3)
    new_bubbles = birange(191,203,4)
    players = birange(210,226,13)
    jumping_players = birange(683,687,4)
    new_players = birange(693,696,3)
    numbers = birange(499,509,10)     # FIXME: already seen below
    comming = birange(693,696,3)
    points = bidict({
        100: 529,
        150: 530,
        200: 531,
        250: 532,
        300: 533,
        350: 534,
        500: 535,
        550: 536,
        600: 537,
        650: 538,
        700: 539,
        750: 540,
        800: 541,
        850: 542,
        900: 543,
        950: 544,
        1000: 545,
        2000: 546,
        3000: 547,
        4000: 548,
        5000: 549,
        6000: 550,
        7000: 551,
        8000: 552,
        9000: 553,
        10000: 20,
        20000: 21,
        30000: 22,
        40000: 23,
        50000: 24,
        60000: 25,
        70000: 26,
        },{
        100: 561,
        150: 562,
        200: 563,
        250: 564,
        300: 565,
        350: 566,
        500: 567,
        550: 568,
        600: 569,
        650: 570,
        700: 571,
        750: 572,
        800: 573,
        850: 574,
        900: 575,
        950: 576,
        1000: 577,
        2000: 578,
        3000: 579,
        4000: 580,
        5000: 581,
        6000: 582,
        7000: 583,
        8000: 584,
        9000: 585,
        10000: 90,
        20000: 91,
        30000: 92,
        40000: 93,
        50000: 94,
        60000: 95,
        70000: 96,
        })
    gameover = birange(497,498,1)
    digits = birange(499,509,10)
    fish = birange(700,707,7)

class Butterfly(Monster1):
    right  = [('butterfly', 'fly', n) for n in range(2)]
    left   = [Bonuses.insect, Bonuses.butterfly]
    jailed = [('butterfly', 'jailed', n) for n in range(3)]
    dead   = [('butterfly', 'dead', n) for n in range(4)]

class Sheep(Monster1):
    right  = [('sheep', 1, n) for n in range(4)]
    left   = [('sheep',-1, n) for n in range(4)]
    right_angry = right
    left_angry  = left
    
