--#############################################################
-- Author   : Stefan Ritt
-- Contents : DRS4 Evaluation Board FPGA top level entity
-- $Id: drs4_eval1.vhd 13988 2009-08-03 15:28:19Z ritt $
--#############################################################

library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.STD_LOGIC_ARITH.ALL;
use IEEE.STD_LOGIC_UNSIGNED.ALL;
use work.drs4_pack.all;

entity drs4_eval1 is
  port (
    -- Quartz
    P_I_CLK33                  : in  std_logic;
    P_I_CLK66                  : in  std_logic;
    
    -- Lemo connectors / test points / trigger
    P_I_LEMO1                  : in  std_logic; 
    P_I_J36                    : in  std_logic; 
    P_I_J37                    : in  std_logic; 
    P_IO_J38                   : inout std_logic; 
    P_IO_J39                   : inout std_logic; 
    P_I_ATRG                   : in  std_logic;

    -- LEDs
    P_O_LED0                   : out std_logic; 
    P_O_LED1                   : out std_logic; 

    -- Lines to/from Cy7C68013A microcontroller
    P_IO_UC_SLOE               : inout std_logic;
    P_IO_UC_SLRD               : inout std_logic;
    P_IO_UC_SLWR               : inout std_logic;
    P_IO_UC_SLCS               : inout std_logic;
    P_IO_UC_PKTEND             : inout std_logic;
    P_IO_UC_FIFOADR0           : inout std_logic;
    P_IO_UC_FIFOADR1           : inout std_logic;
    P_IO_UC_FLAGA              : inout std_logic;
    P_IO_UC_FLAGB              : inout std_logic;
    P_IO_UC_FLAGC              : inout std_logic;
    P_I_UC_PA0                 : in    std_logic;
    
    P_IO_UC_FD                 : inout std_logic_vector(15 downto 0);

    -- PMC connector
    P_IO_PMC_USR               : inout std_logic_vector(63 downto 0)
);
end drs4_eval1;

architecture arch of drs4_eval1 is

  component usr_clocks
    port (
      P_I_CLK33                : in  std_logic; 
      P_I_CLK66                : in  std_logic; 
      O_CLK33                  : out std_logic; 
      O_CLK33_NODLL            : out std_logic; 
      O_CLK66                  : out std_logic; 
      O_CLK132                 : out std_logic; 
      O_CLK264                 : out std_logic; 
      I_PS_VALUE               : in std_logic_vector(7 downto 0);
      O_CLK_PS                 : out std_logic; 
      O_LOCKED                 : out std_logic;
      
      O_DEBUG1                 : out std_logic;
      O_DEBUG2                 : out std_logic
    );
  end component;

  component usb2_racc is
    port (
      -- Clock signals
      -- ------------------------
      I_RESET                  : in std_logic;
      I_CLK33                  : in std_logic;
      
      -- Lines to/from Cy7C68013A microcontroller
      -- -----------------------------------
      P_IO_UC_SLOE             : inout std_logic;
      P_IO_UC_SLRD             : inout std_logic;
      P_IO_UC_SLWR             : inout std_logic;
      P_IO_UC_SLCS             : inout std_logic;
      P_IO_UC_PKTEND           : inout std_logic;
      P_IO_UC_FIFOADR0         : inout std_logic;
      P_IO_UC_FIFOADR1         : inout std_logic;
      P_IO_UC_FLAGA            : inout std_logic;
      P_IO_UC_FLAGB            : inout std_logic;
      P_IO_UC_FLAGC            : inout std_logic;
      P_IO_UC_FD               : inout std_logic_vector(15 downto 0);

      -- Simple bus interface to on-chip RAM
      -- --------------------------------------------------
      O_LOCBUS_ADDR            : out std_logic_vector(31 downto 0);
      I_LOCBUS_D_RD            : in  std_logic_vector(31 downto 0);
      O_LOCBUS_D_WR            : out std_logic_vector(31 downto 0);
      O_LOCBUS_WE              : out std_logic;

      -- Status & control registers
      -----------------------------
      O_CONTROL_REG_ARR        : out type_control_reg_arr;
      I_STATUS_REG_ARR         : in type_status_reg_arr;

      O_CONTROL_TRIG_ARR       : out type_control_trig_arr;
      O_CONTROL0_BIT_TRIG_ARR  : out std_logic_vector(31 downto 0);

      -- Debug signals
      -- -------------
      O_DEBUG                  : out std_logic
    );
  end component;

  component usb_dpram is
    port (
      I_RESET                  : in  std_logic;                       
                                                                     
      I_CLK_A                  : in  std_logic;                       
      I_ADDR_A                 : in  std_logic_vector(31 downto 0);  
      I_WE_A                   : in  std_logic;                      
      O_D_RD_A                 : out std_logic_vector(31 downto 0);  
      I_D_WR_A                 : in  std_logic_vector(31 downto 0);   
                                                                     
      I_CLK_B                  : in  std_logic;                       
      I_ADDR_B                 : in  std_logic_vector(31 downto 0);  
      I_WE_B                   : in  std_logic;                      
      O_D_RD_B                 : out std_logic_vector(31 downto 0);  
      I_D_WR_B                 : in  std_logic_vector(31 downto 0)    

    );
  end component;

  component drs4_eval1_app is
    port (
    
      I_CLK33                  : in std_logic; -- 33 MHz, sychronised to clk33_nodll
      I_CLK66                  : in std_logic; -- 66 MHz, same phase as clk33
      I_CLK132                 : in std_logic; -- 132 MHz, random phase in respect to clk33
      I_CLK264                 : in std_logic; -- 264 MHz, random phase in respect to clk33
      O_CLK_PS_VALUE           : out std_logic_vector(7 downto 0); -- value for phase shift
      I_CLK_PS                 : in std_logic; -- phase shifted in respect to clk33
      I_RESET                  : in std_logic; -- active high power-up reset
  
      -- trigger inputs
      I_TRIGGER1               : in std_logic;
      I_TRIGGER2               : in std_logic;
  
      -- PMC
      P_IO_PMC_USR             : inout std_logic_vector(63 downto 0);

      -- Simple bus interface to DPRAM
      O_DPRAM_CLK              : out std_logic;
      O_DPRAM_ADDR             : out std_logic_vector(31 downto 0);
      O_DPRAM_D_WR             : out std_logic_vector(31 downto 0);
      O_DPRAM_WE               : out std_logic;
      I_DPRAM_D_RD             : in std_logic_vector(31 downto 0);

      -- Control & status registers from system FPGA interface
      I_CONTROL_REG_ARR        : in  type_control_reg_arr;
      O_STATUS_REG_ARR         : out type_status_reg_arr;
      I_CONTROL_TRIG_ARR       : in  type_control_trig_arr;
      I_CONTROL0_BIT_TRIG_ARR  : in  std_logic_vector(31 downto 0);

      -- LEDs signals
      O_LED_RED                : out std_logic;
      O_LED_YELLOW             : out std_logic;
      
      -- Debug signals
      O_DEBUG1                 : out std_logic;
      O_DEBUG2                 : out std_logic
    );
  end component;

  signal VCC: std_logic;
  signal GND: std_logic;

  -- reset signal
  -- -------------
  signal global_reset          : std_logic;  -- active high power-up reset
  
  -- clocks & related signals
  -- ------------------------
  signal clk33_nodll           : std_logic; -- external 33 MHz clock (global clock net)
  signal clk33                 : std_logic; -- 33 MHz DLL output
  signal clk66                 : std_logic;
  signal clk132                : std_logic;
  signal clk264                : std_logic;
  signal clk_ps_value          : std_logic_vector(7 downto 0);
  signal clk_ps                : std_logic; -- special phase shifted clock
  signal usr_clks_dlls_locked  : std_logic; -- high if clock DLLs for clkxx have locked

  -- user application signals for Locbus interface
  -- ---------------------------------------------
  signal locbus_addr           : std_logic_vector(31 downto 0);
  signal locbus_d_rd           : std_logic_vector(31 downto 0);
  signal locbus_d_wr           : std_logic_vector(31 downto 0);
  signal locbus_we             : std_logic;

  -- user application signals for DPRAM interface
  -- --------------------------------------------
  signal dpram_clk             : std_logic;
  signal dpram_addr            : std_logic_vector(31 downto 0);
  signal dpram_we              : std_logic;
  signal dpram_d_wr            : std_logic_vector(31 downto 0);
  signal dpram_d_rd            : std_logic_vector(31 downto 0);

  -- register signals for data exchange with microcontroller
  -- -------------------------------------------------------
  signal control_reg_arr       : type_control_reg_arr;
  signal status_reg_arr        : type_status_reg_arr;

  signal control_trig_arr: type_control_trig_arr;
  signal control0_bit_trig_arr : std_logic_vector(31 downto 0);

  -- LEDs
  -- ----
  signal o_led_red             : std_logic;
  signal o_led_yellow           : std_logic;

  -- Trigger
  -- -------
  signal i_ext_trigger         : std_logic;
  signal i_ana_trigger         : std_logic;

  -- Debugging signals
  -- -----------------
  signal o_racc_debug          : std_logic;
  signal o_debug1              : std_logic;
  signal o_debug2              : std_logic;

begin
  VCC <= '1';
  GND <= '0';

  -- map LEDs
  P_O_LED0      <= o_led_yellow;
  P_O_LED1      <= o_led_red;
  
  -- debug outputs
  P_IO_J38      <= GND;
  P_IO_J39      <= GND;

  i_ext_trigger <= P_I_LEMO1;
  i_ana_trigger <= P_I_ATRG;

  clocks : usr_clocks port map (
    P_I_CLK33                  => P_I_CLK33,
    P_I_CLK66                  => P_I_CLK66,
    O_CLK33                    => clk33,              
    O_CLK33_NODLL              => clk33_nodll,        
    O_CLK66                    => clk66,              
    O_CLK132                   => clk132,             
    O_CLK264                   => clk264,  
    I_PS_VALUE                 => clk_ps_value,
    O_CLK_PS                   => clk_ps,           
    O_LOCKED                   => usr_clks_dlls_locked,
    
    O_DEBUG1                   => o_debug1,
    O_DEBUG2                   => o_debug2
  );

  -- global system FPGA reset (synchronous): Assert reset until DLL
  -- for clock DLLs have locked
  proc_reset: process(clk33_nodll, usr_clks_dlls_locked)
  begin
    if (usr_clks_dlls_locked = '0') then
      global_reset        <= '1';
    elsif rising_edge(clk33_nodll) then
      if (P_I_UC_PA0 = '0') then -- reset with uC PA0 line
        global_reset      <= '1';
      else
        global_reset      <= '0';
      end if;  
    end if;
  end process;

  -- interface to status & control registers
  -- ---------------------------------------
  usb2_racc_interface: usb2_racc
    port map (
      I_RESET                  => global_reset, 
      I_CLK33                  => clk33,                   
                                                           
      P_IO_UC_SLOE             => P_IO_UC_SLOE,                 
      P_IO_UC_SLRD             => P_IO_UC_SLRD,
      P_IO_UC_SLWR             => P_IO_UC_SLWR,
      P_IO_UC_SLCS             => P_IO_UC_SLCS,
      P_IO_UC_PKTEND           => P_IO_UC_PKTEND,
      P_IO_UC_FIFOADR0         => P_IO_UC_FIFOADR0,
      P_IO_UC_FIFOADR1         => P_IO_UC_FIFOADR1,
      P_IO_UC_FLAGA            => P_IO_UC_FLAGA,
      P_IO_UC_FLAGB            => P_IO_UC_FLAGB,
      P_IO_UC_FLAGC            => P_IO_UC_FLAGC,
      P_IO_UC_FD               => P_IO_UC_FD,

      O_LOCBUS_ADDR            => locbus_addr,
      I_LOCBUS_D_RD            => locbus_d_rd,
      O_LOCBUS_D_WR            => locbus_d_wr,
      O_LOCBUS_WE              => locbus_we,               
                                                           
      O_CONTROL_REG_ARR        => control_reg_arr,         
      I_STATUS_REG_ARR         => status_reg_arr,          
                                                           
      O_CONTROL_TRIG_ARR       => control_trig_arr,        
      O_CONTROL0_BIT_TRIG_ARR  => control0_bit_trig_arr,
      
      O_DEBUG                  => o_racc_debug
    );

  -- DPRAM (FIFO) for waveform storage
  -- ---------------------------------
  drs_dpram : usb_dpram
    port map (
      I_RESET                  => global_reset,
                                                 
      I_CLK_A                  => clk66,         
      I_ADDR_A                 => locbus_addr,   
      I_WE_A                   => locbus_we,     
      O_D_RD_A                 => locbus_d_rd,   
      I_D_WR_A                 => locbus_d_wr,   
                                                 
      I_CLK_B                  => dpram_clk,      
      I_ADDR_B                 => dpram_addr,     
      I_WE_B                   => dpram_we,       
      O_D_RD_B                 => dpram_d_rd,     
      I_D_WR_B                 => dpram_d_wr      
                                                 
    );

  -- user PMC application 
  -- --------------------
  app : drs4_eval1_app
    port map (
      I_RESET                  => global_reset,        
      I_CLK33                  => clk33,                 
      I_CLK66                  => clk66,                 
      I_CLK132                 => clk132,                
      I_CLK264                 => clk264,                
      O_CLK_PS_VALUE           => clk_ps_value,
      I_CLK_PS                 => clk_ps,              
       
      I_TRIGGER1               => i_ext_trigger,
      I_TRIGGER2               => i_ana_trigger,
      
      P_IO_PMC_USR             => P_IO_PMC_USR,          
                                                         
      O_DPRAM_CLK              => dpram_clk,              
      O_DPRAM_ADDR             => dpram_addr,             
      O_DPRAM_D_WR             => dpram_d_wr,             
      O_DPRAM_WE               => dpram_we,
      I_DPRAM_D_RD             => dpram_d_rd,               
                                                         
      I_CONTROL_REG_ARR        => control_reg_arr,       
      O_STATUS_REG_ARR         => status_reg_arr,        
                                                         
      I_CONTROL_TRIG_ARR       => control_trig_arr,      
      I_CONTROL0_BIT_TRIG_ARR  => control0_bit_trig_arr,

      O_LED_RED                => o_led_red,
      O_LED_YELLOW             => o_led_yellow,
      
      O_DEBUG1                 => open,
      O_DEBUG2                 => open
      );
  
end arch;
