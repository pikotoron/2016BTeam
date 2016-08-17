#include "app.h"
#include "DD_Gene.h"
#include "DD_RCDefinition.h"
#include "SystemTaskManager.h"
#include <stdlib.h>
#include "message.h"
#include "MW_GPIO.h"

#define _MIN(x, y) (( x ) < ( y ) ? ( x ) : ( y ))
#define _MAX(x, y) (( x ) > ( y ) ? ( x ) : ( y ))

/*suspensionSystem*/
static
int suspensionSystem(void);

/*ABSystem*/
static
int ABSystem(void);

/*メモ
 * g_ab_h...ABのハンドラ
 * g_md_h...MDのハンドラ
 *
 * g_rc_data...RCのデータ
 */

int appInit(void){
  message("msg", "Message");
  /*GPIO の設定などでMW,GPIOではHALを叩く*/
  return EXIT_SUCCESS;
}

/*application tasks*/
int appTask(void){
  int ret = 0;

  /*それぞれの機構ごとに処理をする*/
  /*途中必ず定数回で終了すること。*/
  ret = suspensionSystem();
  if( ret ){
    return ret;
  }

  ret = ABSystem();
  if( ret ){
    return ret;
  }

  return EXIT_SUCCESS;
}

static
int ABSystem(void){
  g_ab_h[0].dat = 0x00;
  if( __RC_ISPRESSED_CIRCLE(g_rc_data)){
    g_ab_h[0].dat |= AB0;
  }
  if( __RC_ISPRESSED_CROSS(g_rc_data)){
    g_ab_h[0].dat |= AB1;
  }

  return EXIT_SUCCESS;
}

/*プライベート 足回りシステム*/
static
int suspensionSystem(void){
  const int num_of_motor = 2;/*モータの個数*/
  const int inc_c = 200;/*Duty上昇時の傾き*/
  const int dec_c = 200;/*Duty下降時の傾き*/
  int target_duty;/*目標値となるDuty*/
  int prev_duty;/*直前のDuty*/
  int sd_duty = 0;
  int rc_analogdata;/*アナログデータ*/
  unsigned int idx;/*インデックス*/
  int i;

  /*for each motor*/
  for( i = 0; i < num_of_motor; i++ ){
    /*それぞれの差分*/
    switch( i ){
    case 0:
      rc_analogdata = DD_RCGetRY(g_rc_data);
      idx = MECHA1_MD1;
      if ( _IS_REVERSE_R ){
	rc_analogdata = -rc_analogdata;
      }
      break;
    case 1:
      rc_analogdata = DD_RCGetLY(g_rc_data);
      idx = MECHA1_MD2;
      if ( _IS_REVERSE_L ){
	rc_analogdata = -rc_analogdata;
      }
      break;
    default: return EXIT_FAILURE;
    }

    /*これは中央か?±3程度余裕を持つ必要がある。*/
    if( abs(rc_analogdata) < CENTRAL_THRESHOLD ){
      target_duty = 0;
    } else{
      target_duty = rc_analogdata * MD_GAIN;
    }

    prev_duty = g_md_h[idx].duty;
    if( g_md_h[idx].mode == D_MMOD_FORWARD ){/*直前が正回転なら*/
      if( target_duty > prev_duty ){
        sd_duty = prev_duty + _MIN(inc_c, target_duty - prev_duty);
      } else{
        sd_duty = prev_duty + _MAX(-dec_c, target_duty - prev_duty);
      }
    }else {/*直前が逆回転 or Freeなら*/
      if( target_duty < -prev_duty ){
        sd_duty = -prev_duty + _MAX(-inc_c, target_duty - ( -prev_duty ));
      } else{
        sd_duty = -prev_duty + _MIN(dec_c, target_duty - ( -prev_duty ));
      }
    }
    if( sd_duty > 0 ){
      g_md_h[idx].mode = D_MMOD_FORWARD;
    } else if( sd_duty < 0 ){
      g_md_h[idx].mode = D_MMOD_BACKWARD;
    } else{
      g_md_h[idx].mode = D_MMOD_FREE;
    }
    g_md_h[idx].duty = abs(sd_duty);
  }

  return EXIT_SUCCESS;
} /* suspensionSystem */
