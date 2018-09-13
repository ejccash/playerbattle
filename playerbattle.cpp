#include <eosiolib/eosio.hpp>
#include <eosiolib/print.hpp>
#include <eosiolib/currency.hpp>
#include <math.h>
#include <eosiolib/crypto.h>
#include <eosiolib/transaction.hpp>

using namespace eosio;



#define EJC_SYMBOL S(4, EJC)
#define GAME_SYMBOL S(4, EOS)
#define DEV_FEE_ACCOUNT N(eosjoycenter)
#define TOKEN_CONTRACT N(eosio.token)

//空投EJC
#define L_TOKEN_SYMBOL S(4, EJC)
#define L_TOKEN_CONTRACT N(ejcoinstoken)

class playerbattle : public eosio::contract {
  public:    
      playerbattle(account_name self)
      : contract(self),_games(self,self),_currentgames(self,self),_players(self,self){}

    /***
     * 游戏信息
     * */
    /// @abi table
    struct game{
        uint64_t id;
        account_name creater;   //游戏创建人  创建人需要支付至少1 EOS押金作为游戏房间创建费用，如果房间没有人来玩，creater可以选择关闭房间、拿回押金。但是如果有人来玩，而房主不应战(如房主支付超时，防止恶意创建游戏)，则会扣除压金给玩家。
                                //房主押EOS 游戏正常开始后（至少经过１轮)，房主不管输赢均能拿回压金。房主压金量同时也规定了玩家起始压注量的最小值，同时玩家初始投注量也不可以大于该值的３倍
                                //房主虽然支付压金开始游戏，同时也占有一定的后手优势。（可以看玩家实力而选择应战或者不应战）且在游戏正常结束后可退回押金，所以两相持平，creater并不吃亏。
                                //玩家可以选择房主，但同时也需要先出牌。所以优略互补，也不吃亏。
                                //create 需要支出一定的内存
        account_name player;    //游戏参与人，参与人根据creater 押金量首先出牌(不可以少于押金量，但不能高于３倍)
        asset deposit;             //creater 押金支付用于创建房间的费用（可退回的)
        asset ctotalpay;        //游戏回合阶段创建者总支付数
        asset ptotalpay;        //游戏回合阶段玩家总计支付                
        uint32_t cscore;        //创建者总分数
        uint32_t pscore;        //玩家总分数
                                //分数计算：玩家每一次压注都会由合约生成１-13的一个数字代表扑克的A-K,分数归定:
                                // A 1分， 2 2分， 3 3分， 4 4分， 5 5分， 6 6分， 7 7分， 8 8分， 9 9分， 10 10分， J 11分， Q 12分， K 13分
        uint8_t steps;         // 玩家首次出牌后变为１，之后每一次出牌都会将该值+1.
                                // steps 最大值为６,（也就是３回合  每回合均以player出牌为开始，以creater出牌为结束)
                                // 当达到６时（creater最后一次支付后，合约自动计算双方得分，判断最后输赢)
                                // 回合结束时 分数高者为胜，分数低者为负。
                                // creater 胜:  得到player所有压注量的90%,10%为合约开发维护费用；收回自己的所有压注；收回自己的押金。
                                // player 胜: 得到creater所有压注量的90%，10％为合约开发维护费用。收回自己的所有压注；creater 收回自己的游戏押金。
                                // 回合结束时 分数相同判为平，双方各拿回自己的投注。creater拿回押金
        asset lastpay;          //最近一次压注量（本次压注要大于等于该值，但最大不能超过３倍)
        uint64_t lasttime;      //最近一次支付时间（本次支付时间不可超过该时间15分钟，超时判负）
        uint8_t status;         //游戏状态  0:初始 等待玩家参与  1:正在进行  2:已结束
        account_name winner;    //最终胜者,最终胜者可能是分高者，也可能是对方放弃游戏
        uint64_t primary_key() const { return id; }
        EOSLIB_SERIALIZE(game,(id)(creater)(player)(deposit)(ctotalpay)(ptotalpay)(cscore)(pscore)(steps)(lastpay)(lasttime)(status)(winner));
    };
    
    typedef eosio::multi_index<N(game),game> games;
    games _games;

    /**
     * 正在进行中，所有未结束的游戏。
     * */
    /// @abi table
    struct currentgame{
        uint64_t id;            //正在进行的游戏id. 游戏结束或者提前结束均从该表中删除
        account_name creater;   //创建者，防止重复创建，也可快速定位自己游戏。
        account_name player;    //参与者
        asset deposit;          //房间压金，起始投注的最小值
        uint8_t status;         //状态　　用户自己参与的　要排在最前面　然后是状态为0的　　最后是其他
        uint8_t steps;          //步数   
        uint64_t lasttime;      //游戏开始后最新的支付时间，以便于显示超时时长（方便其他玩家清理超时的游戏以获得一定的奖励）
        uint64_t primary_key() const { return id; }
        account_name getcreater() const{ return creater;}
        EOSLIB_SERIALIZE(currentgame,(id)(creater)(player)(deposit)(status)(steps)(lasttime))
    };
    typedef eosio::multi_index<N(currentgame),currentgame,
         eosio::indexed_by<N(bycreater),eosio::const_mem_fun<currentgame,account_name,&currentgame::getcreater>>
    > currentgames;
    currentgames   _currentgames;
    
    /**
     * 玩家统计信息
     * */
    /// @abi table
    struct player{
        account_name account;
        uint64_t lost;     //总输局数
        uint64_t win;      //总胜局数
        uint64_t draw;      //平局数
        asset pay;         //总投入
        asset payout;      //总收入
        account_name primary_key() const { return account; }
        EOSLIB_SERIALIZE(player,(account)(lost)(win)(draw)(pay)(payout))
    };
    typedef eosio::multi_index<N(player),player> players;
    players _players;
    /**
     * 用户可以调用该action 以查检随机数生成器公平性。
    */
    /// @abi action
    void roll(){
        print(getscore());
    } 
    /**
     * 产生1-13 的随机数，表示得分，为了保证合约安全性及公平性，该方法暂不开源，将来可以视玩家要求而选择开源。
    */
    const uint32_t getscore(){
      
    }

    void creategame(account_name creater,asset deposit){
        eosio_assert(deposit.symbol == GAME_SYMBOL,"only accepte EOS to create game ");
        eosio_assert(deposit.amount >= 10000,"minimum 1 EOS to create new game");
        auto creater_index = _currentgames.get_index<N(bycreater)>();
        auto cg_itr = creater_index.find(creater);
        eosio_assert(cg_itr == creater_index.end(),"you had create a game! ");

        uint64_t gameid = _games.available_primary_key();
      
        _games.emplace(_self,[&](auto &g){
            g.id=gameid;
            g.creater = creater;        
            g.deposit = deposit;
            g.steps =  0;
            g.status = 0;
            g.lastpay = deposit;
            g.pscore = 0;
            g.cscore = 0;
            g.ctotalpay = asset(0,GAME_SYMBOL);
            g.ptotalpay = asset(0,GAME_SYMBOL);
        });
        _currentgames.emplace(_self,[&](auto &cg){
            cg.id=gameid;
            cg.creater=creater;
            cg.deposit = deposit;
            cg.status = 0;
            cg.steps = 0; 
        });
    }

    /**
     * 游戏开始后，参与双方都不再出牌（游戏状态为１,参与人也不主动关闭游戏）至时游戏长时间得不到关闭，超时120分钟以上，可由第三方玩家(非creater或player)调用该合约方法关闭游戏。
     * 第三方玩家成功清理游戏后将获得游戏双方总投注（包括游戏创建者的押金)的90％的奖励，10%为合约开发费用
     * 此举是为了，防止有玩家恶意攻击合约（创建大量无用游戏，占用宝贵的合约内存资源,同时还影响其他玩家游戏体验)。
     * 这里加重了对恶意攻击合约的惩罚，恶意创建游戏，占用游戏资源将使攻击者血本无归。
     * 对维护合约正常，清理无用游戏玩家奖励丰厚。
     * 
     * */
     /// @abi action
    void cleargame(account_name from,uint32_t id){
        require_auth(from);
        auto g_itr = _games.find(id);
        eosio_assert(g_itr != _games.end(),"cannt find the game ");
        eosio_assert(g_itr->status == 1," you cannt clear this game !");
        eosio_assert(now()-g_itr->lasttime > 120*60," the game has not time out! you cannt clear this game!");        
        eosio_assert(g_itr->creater!= from && g_itr->player != from," you can involve claim() or giveup() to clear this game");


        asset payout = (g_itr->deposit + g_itr->ctotalpay + g_itr->ptotalpay)/100 * 90 ;
        asset devpayout = (g_itr->deposit + g_itr->ctotalpay + g_itr->ptotalpay)/10;
        action(
            permission_level{_self,N(active)},
            TOKEN_CONTRACT,N(transfer),
            make_tuple(_self,from,payout,string("good job!reward from battle.ejc.cash!"))
        ).send();      

        action(
            permission_level{_self,N(active)},
            TOKEN_CONTRACT,N(transfer),
            make_tuple(_self,DEV_FEE_ACCOUNT,devpayout,string("dev fee from battle.ejc.cash!"))
        ).send();

        _games.erase(g_itr);
        auto cg_itr = _currentgames.find(id);
        if(cg_itr != _currentgames.end()){
            _currentgames.erase(cg_itr);
        }

    }

    /**
     * 关闭游戏,必須由游戏创建者关闭，且游戏没有player参与时才可关闭，退回押金，并且删除_games表信息
     * */
    /// @abi action
    void closegame(account_name from,uint32_t id){
        require_auth(from);
     
        auto g_itr = _games.find(id);
        eosio_assert(g_itr != _games.end(),"cannt find your game ");
        eosio_assert(g_itr->creater == from ," this game is not created by you");
        eosio_assert(g_itr->status == 0," you cannt stop this game !");

        
        action(
            permission_level{_self,N(active)},
            TOKEN_CONTRACT,N(transfer),
            make_tuple(_self,g_itr->creater,g_itr->deposit,string("deposit from battle.ejc.cash!"))
        ).send();          

        _games.erase(g_itr);
        auto cg_itr = _currentgames.find(id);
        if(cg_itr != _currentgames.end()){
            _currentgames.erase(cg_itr);
        }
    }
    /**
     * 申请结束游戏。（游戏开始后非当前回合参与者申请结束游戏。对方支付超时（视对方放弃游戏）可以调用该方法结束游戏，并取得胜利，拿到胜利奖金）
     * */
    /// @abi action
    void claim(account_name from,uint32_t id){
        require_auth(from);

        auto g_itr = _games.find(id);
        eosio_assert(g_itr != _games.end(),"cann't find your game ");
        eosio_assert(g_itr->status == 1," your cannt claim this game!");
        eosio_assert(now()-g_itr->lasttime>15*60," not time out ,you cannt do this!"); //检查是否超时
        if(g_itr ->steps == 1){
            //　1 的时候由player　claim 游戏，说明creater超时，此时为creater 败，应将deposit给player，（因为steps 为１时，creater　还没有出牌，只能将押金给玩家）
            eosio_assert(from == g_itr->player,"you cannt do this!");
            asset playerpayout = g_itr->deposit/100*90 + g_itr->ptotalpay;    //奖励90%的creater押金deposit。 
            asset devfee = g_itr->deposit /10;        //10%合约开发维护费用
      

            action(
                permission_level{_self,N(active)},
                TOKEN_CONTRACT,N(transfer),
                make_tuple(_self,g_itr->player,playerpayout,string("winner of battle game! reward from battle.ejc.cash!"))
            ).send();      

            action(
                permission_level{_self,N(active)},
                TOKEN_CONTRACT,N(transfer),
                make_tuple(_self,DEV_FEE_ACCOUNT,devfee,string("dev fee from battle.ejc.cash!"))
            ).send();

        
            //更新creater游戏信息
            auto c_itr = _players.find(g_itr->creater);
            if(c_itr == _players.end()){
                _players.emplace(_self,[&](auto &p){                                
                    p.account = g_itr->creater;
                    p.lost = 1;
                    p.win = 0;
                    p.draw = 0;
                    p.pay = g_itr->deposit ;
                    p.payout =  asset(0,GAME_SYMBOL);
                });
            }else{
                _players.modify(c_itr,_self,[&](auto &p){
                    p.lost+=1;
                    p.pay+= g_itr->deposit ;                   
                });
            }
            
            //更新player游戏信息
            auto p_itr = _players.find(g_itr->player);
            if(p_itr == _players.end()){
                _players.emplace(_self,[&](auto &p){
                    p.account = g_itr->player;
                    p.lost =0;
                    p.win =1;
                    p.draw =0;
                    p.pay = g_itr->ptotalpay;
                    p.payout = playerpayout;
                });
            }else{
                _players.modify(p_itr,_self,[&](auto &p){
                    p.win += 1;
                    p.pay += g_itr->ptotalpay;
                    p.payout += playerpayout;
                });
            }

        }else if(g_itr->steps ==3 || g_itr->steps ==5){
            //如果步数是３、５ 说明应该是creater出牌。如果creater超时，player可以进行claim操作
            eosio_assert(from == g_itr->player,"you cannt do this!");
            asset playerpayout = g_itr->ctotalpay /100*90 + g_itr->ptotalpay;    //奖励90%的creater投入以及自己的全部投入 ，10%内存、开发、维护开消
            asset devfee = g_itr->ctotalpay /10;        //10%合约开发维护费用
            asset createrpayout = g_itr->deposit; //游戏押金。
            action(
                permission_level{_self,N(active)},
                TOKEN_CONTRACT,N(transfer),
                make_tuple(_self,g_itr->player,playerpayout,string("winner of battle game! reward from battle.ejc.cash!"))
            ).send();      

            action(
                permission_level{_self,N(active)},
                TOKEN_CONTRACT,N(transfer),
                make_tuple(_self,DEV_FEE_ACCOUNT,devfee,string("dev fee from battle.ejc.cash!!!"))
            ).send();

            action(
                permission_level{_self,N(active)},
                TOKEN_CONTRACT,N(transfer),
                make_tuple(_self,g_itr->creater,createrpayout,string("deposit from battle.ejc.cash"))
            ).send();

            //更新creater游戏信息
            auto c_itr = _players.find(g_itr->creater);
            if(c_itr == _players.end()){
                _players.emplace(_self,[&](auto &p){                                
                    p.account = g_itr->creater;
                    p.lost = 1;
                    p.win = 0;
                    p.draw = 0;
                    p.pay = g_itr->ctotalpay ;
                    p.payout = asset(0,GAME_SYMBOL); //只能拿加投入的2%
                });
            }else{
                _players.modify(c_itr,_self,[&](auto &p){
                    p.lost+=1;
                    p.pay+= g_itr->ctotalpay ;                    
                });
            }
            //更新player游戏信息
            auto p_itr = _players.find(g_itr->player);
            if(p_itr == _players.end()){
                _players.emplace(_self,[&](auto &p){
                    p.account = g_itr->player;
                    p.lost =0;
                    p.win =1;
                    p.draw =0;
                    p.pay = g_itr->ptotalpay;
                    p.payout = playerpayout;
                });
            }else{
                _players.modify(p_itr,_self,[&](auto &p){
                    p.win += 1;
                    p.pay += g_itr->ptotalpay;
                    p.payout += playerpayout;
                });
            }
        }else if(g_itr->steps ==2 || g_itr->steps == 4){
            //如果步数是2、４ 说明应该是player出牌，如果player超时，creater可以进行claim操作。
            eosio_assert(from == g_itr->creater,"you cannt do this!");

            asset devfee = g_itr->ptotalpay/10;        //10%合约开发维护费用
            asset createrpayout = g_itr->ptotalpay/100*90 + g_itr->deposit + g_itr->ctotalpay ; //player的90% ＋ 押金+投入
    
            action(
                permission_level{_self,N(active)},
                TOKEN_CONTRACT,N(transfer),
                make_tuple(_self,DEV_FEE_ACCOUNT,devfee,string("dev fee from battle.ejc.cash!!!"))
            ).send();

            action(
                permission_level{_self,N(active)},
                TOKEN_CONTRACT,N(transfer),
                make_tuple(_self,g_itr->creater,createrpayout,string("winner of battle game! reward from battle.ejc.cash!"))
            ).send();


                //更新creater游戏信息
            auto c_itr = _players.find(g_itr->creater);
            if(c_itr == _players.end()){
                _players.emplace(_self,[&](auto &p){                                
                    p.account = g_itr->creater;
                    p.lost = 0;
                    p.win = 1;
                    p.draw = 0;
                    p.pay = g_itr->ctotalpay;
                    p.payout = createrpayout - g_itr->deposit; //押金不计入总投入，自然也不计入总收入。
                });
            }else{
                _players.modify(c_itr,_self,[&](auto &p){
                    p.win+=1;
                    p.pay+= g_itr->ctotalpay;
                    p.payout += (createrpayout - g_itr->deposit); //同上收入中要排除押金。
                });
            }

            //更新player游戏信息
            auto p_itr = _players.find(g_itr->player);
            if(p_itr == _players.end()){
                _players.emplace(_self,[&](auto &p){
                    p.account = g_itr->player;
                    p.lost =1;
                    p.win =0;
                    p.draw =0;
                    p.pay = g_itr->ptotalpay;
                    p.payout = asset(0,GAME_SYMBOL);
                });
            }else{
                _players.modify(p_itr,_self,[&](auto &p){
                    p.lost += 1;
                    p.pay += g_itr->ptotalpay;
                });
            }
        }
         //更新游戏状态至结束
        _games.modify(g_itr,_self,[&](auto &g){         
            g.status = 2; //表示游戏已结束；
            if(g_itr->steps ==1 || g_itr->steps==3 || g_itr->steps==5){
                g.winner = g_itr->player;
            }else{
                g.winner = g_itr->creater;
            }
        });

        //从正在进行游戏列表中将游戏清除
        auto cg_itr = _currentgames.find(id);
        if(cg_itr != _currentgames.end()){
            _currentgames.erase(cg_itr);
        }
    }
    /**
     * 当前回合参与者宣布投降。
     * **/
    /// @abi action
    void giveup(account_name from ,uint32_t id){
        //回合内的一方可以选择放弃游戏（如对方优势过大，已没有翻盘可能，可以选择放弃认输。
        require_auth(from);
        auto g_itr = _games.find(id);
        eosio_assert(g_itr != _games.end(),"cann't find your game ");
        eosio_assert(g_itr->status ==1," your cannt give up this game!");
        //超时后也可以调用giveup 结束游戏让对方胜。
        if(g_itr->steps == 1){
            //当step为1时creater放弃游戏。。（此时因为creater没有压注，需要扣除creater压金给player，防止creater恶意创建房间）
            eosio_assert(g_itr->creater == from,"you cannt give up this game! not your turn!");
            asset playerpayout = g_itr->deposit/100*90 + g_itr->ptotalpay;    //奖励90%的creater押金deposit。 
            asset devfee = g_itr->deposit /10;        //10%合约开发维护费用
     

            action(
                permission_level{_self,N(active)},
                TOKEN_CONTRACT,N(transfer),
                make_tuple(_self,g_itr->player,playerpayout,string("winner of battle game! reward from battle.ejc.cash!"))
            ).send();      

            action(
                permission_level{_self,N(active)},
                TOKEN_CONTRACT,N(transfer),
                make_tuple(_self,DEV_FEE_ACCOUNT,devfee,string("dev fee from battle.ejc.cash!"))
            ).send();

           
            //更新creater游戏信息
            auto c_itr = _players.find(g_itr->creater);
            if(c_itr == _players.end()){
                _players.emplace(_self,[&](auto &p){                                
                    p.account = g_itr->creater;
                    p.lost = 1;
                    p.win = 0;
                    p.draw = 0;
                    p.pay = g_itr->deposit ;
                    p.payout =  asset(0,GAME_SYMBOL);   
                });
            }else{
                _players.modify(c_itr,_self,[&](auto &p){
                    p.lost+=1;
                    p.pay+= g_itr->deposit ;
                   
                });
            }
            
            //更新player游戏信息
            auto p_itr = _players.find(g_itr->player);
            if(p_itr == _players.end()){
                _players.emplace(_self,[&](auto &p){
                    p.account = g_itr->player;
                    p.lost =0;
                    p.win =1;
                    p.draw =0;
                    p.pay = g_itr->ptotalpay;
                    p.payout = playerpayout;
                });
            }else{
                _players.modify(p_itr,_self,[&](auto &p){
                    p.win += 1;
                    p.pay += g_itr->ptotalpay;
                    p.payout += playerpayout;
                });
            }

        }else if(g_itr-> steps ==3 || g_itr->steps ==5 ){
            //steps 为３、５ 说明此时为creater 出牌，creater可以选择放弃
            eosio_assert(g_itr->creater == from," you cannt give up this game! not your turn !");
            //creater 放弃后 player胜
            asset playerpayout = g_itr->ctotalpay /100*90 + g_itr->ptotalpay;    //奖励90%的creater投入以及自己的全部投入 ，2%给creater 补偿内存费用，10%开发维护费用。 
            asset devfee = g_itr->ctotalpay /10;        //10%合约开发维护费用
            asset createrpayout = g_itr->deposit; //游戏押金。
            action(
                permission_level{_self,N(active)},
                TOKEN_CONTRACT,N(transfer),
                make_tuple(_self,g_itr->player,playerpayout,string("winner of battle game! reward from battle.ejc.cash!"))
            ).send();      

            action(
                permission_level{_self,N(active)},
                TOKEN_CONTRACT,N(transfer),
                make_tuple(_self,DEV_FEE_ACCOUNT,devfee,string("dev fee from battle.ejc.cash!"))
            ).send();

            action(
                permission_level{_self,N(active)},
                TOKEN_CONTRACT,N(transfer),
                make_tuple(_self,g_itr->creater,createrpayout,string("deposit from battle.ejc.cash!"))
            ).send();

            //更新creater游戏信息
            auto c_itr = _players.find(g_itr->creater);
            if(c_itr == _players.end()){
                _players.emplace(_self,[&](auto &p){                                
                    p.account = g_itr->creater;
                    p.lost = 1;
                    p.win = 0;
                    p.draw = 0;
                    p.pay = g_itr->ctotalpay ;
                    p.payout =  g_itr->ctotalpay/50;   //只能拿加投入的2%
                });
            }else{
                _players.modify(c_itr,_self,[&](auto &p){
                    p.lost+=1;
                    p.pay+= g_itr->ctotalpay ;
                    p.payout += g_itr->ctotalpay/50;   //能拿加投入的2%
                });
            }
            
            //更新player游戏信息
            auto p_itr = _players.find(g_itr->player);
            if(p_itr == _players.end()){
                _players.emplace(_self,[&](auto &p){
                    p.account = g_itr->player;
                    p.lost =0;
                    p.win =1;
                    p.draw =0;
                    p.pay = g_itr->ptotalpay;
                    p.payout = playerpayout;
                });
            }else{
                _players.modify(p_itr,_self,[&](auto &p){
                    p.win += 1;
                    p.pay += g_itr->ptotalpay;
                    p.payout += playerpayout;
                });
            }

        }else if(g_itr->steps == 2 || g_itr->steps == 4){
            eosio_assert(g_itr->player == from,"you cannt give up this game! not your turn!");

            asset devfee = g_itr->ptotalpay/10;        //10%合约开发维护费用
            asset createrpayout = g_itr->ptotalpay/100*90 + g_itr->deposit + g_itr->ctotalpay ; //player的90% ＋ 押金+投入
    
            action(
                permission_level{_self,N(active)},
                TOKEN_CONTRACT,N(transfer),
                make_tuple(_self,DEV_FEE_ACCOUNT,devfee,string("dev fee from battle.ejc.cash!"))
            ).send();

            action(
                permission_level{_self,N(active)},
                TOKEN_CONTRACT,N(transfer),
                make_tuple(_self, g_itr->creater,createrpayout,string("winner of battle game! reward from battle.ejc.cash!"))
            ).send();


                //更新creater游戏信息
            auto c_itr = _players.find( g_itr->creater);
            if(c_itr == _players.end()){
                _players.emplace(_self,[&](auto &p){                                
                    p.account =  g_itr->creater;
                    p.lost = 0;
                    p.win = 1;
                    p.draw = 0;
                    p.pay = g_itr->ctotalpay;
                    p.payout = createrpayout - g_itr->deposit;   //押金不计入总投入，自然也不计入总收入。
                });
            }else{
                _players.modify(c_itr,_self,[&](auto &p){
                    p.win+=1;
                    p.pay+= g_itr->ctotalpay;
                    p.payout += (createrpayout - g_itr->deposit);   //同上收入中要排除押金。
                });
            }

            //更新player游戏信息
            auto p_itr = _players.find(g_itr->player);
            if(p_itr == _players.end()){
                _players.emplace(_self,[&](auto &p){
                    p.account = g_itr->player;
                    p.lost =1;
                    p.win =0;
                    p.draw =0;
                    p.pay = g_itr->ptotalpay;
                    p.payout = asset(0,GAME_SYMBOL);
                });
            }else{
                _players.modify(p_itr,_self,[&](auto &p){
                    p.lost += 1;
                    p.pay += g_itr->ptotalpay;
                });
            }

        }
         //更新游戏状态至结束
        _games.modify(g_itr,_self,[&](auto &g){         
            g.status = 2; //表示游戏已结束；
            if(g_itr->steps ==1 || g_itr->steps==3 || g_itr->steps==5){
                g.winner = g_itr->player;
            }else{
                g.winner = g_itr->creater;
            }
        });

        //从正在进行游戏列表中将游戏清除
        auto cg_itr = _currentgames.find(id);
        if(cg_itr != _currentgames.end()){
            _currentgames.erase(cg_itr);
        }

    }
    /**
     * 清理历史游戏信息。主要是为了释放游戏内存资源，不定期清理对玩家无用的游戏历史数据。
     * */
 /// @abi action 
    void clear(uint32_t n){
        require_auth(_self);

        auto it = _games.begin(); 
        
        while (it != _games.end() && n>0)
        {
            if(it->status ==2){
                it = _games.erase(it);                 
                n--;
            }else{
                it++;
            }
        }
    }
    /// memo 信息应该包括: 游戏id
    /// 进行权限检查

    /// @abi action 
    void transfer(account_name from, account_name to, asset quantity, string memo)
    {   
        require_auth(from);      
        if(from == _self || to != _self){
            return;
        }
        eosio_assert(quantity.symbol == GAME_SYMBOL,"only accepte EOS ");
        if(memo == string("create")){
            //如果memo 是create  创建游戏
            creategame(from,quantity);
        }else{
            //否则memo 将是一个游戏id，表示参与该游戏
            auto g_itr = _games.find(atoi(memo.c_str()));            
            eosio_assert(g_itr != _games.end(),"unknown game!");
            eosio_assert(g_itr -> steps <6,"max step reached");  
            eosio_assert(g_itr -> status != 2,"the game is over.");   

            //airdrop

            //空投 EJC
            asset r = asset(quantity.amount/5,L_TOKEN_SYMBOL);
            action(
                permission_level{_self, N(active)},
                L_TOKEN_CONTRACT, N(transfer),       
                make_tuple(_self,from,r,string("Enjoy Airdrop! EJC From EJC.CASH"))
            ).send();  

            if(g_itr->steps != 0){
                //如果不为0 ，支付者应该为游戏参与的其中一方
                eosio_assert(from == g_itr->creater  || from == g_itr->player,"this is not your game!");            
                //说明游戏已经开始，需要检查是否超时
                eosio_assert(now()-g_itr->lasttime < 15*60,"your pay coming too late ,the game is over!");

                 eosio_assert(quantity.amount >= g_itr->lastpay.amount && quantity.amount <= 3*g_itr->lastpay.amount,"error amount of eos ");
            }else{
                //如果为０,支付者不能为creater                
                eosio_assert(from != g_itr->creater ,"you cannt play this game because you are creater of it!");
                //支付金额要大于等于creater deposit量
                 eosio_assert(quantity.amount >= g_itr->deposit.amount && quantity.amount <= g_itr->deposit.amount * 3,"error amount of eos ");
            }

            if(g_itr->steps ==1 || g_itr->steps ==3 || g_itr->steps ==5){
                //此时为creater支付,creater 最后一次支付后要对游戏进行结算
                eosio_assert(from == g_itr->creater,"it's not your trun yet");
                uint32_t score = getscore();
                if(g_itr->steps !=5){
                    //不等于５ 说明还不是creater最后一次支付（最后一次支付成功后需要进行清算）
                    _games.modify(g_itr,_self,[&](auto &g){
                        g.ctotalpay += quantity;
                        g.cscore += score;
                        g.steps +=1;
                        g.lastpay = quantity;
                        g.lasttime=now();                        
                    });

                     auto cg_itr = _currentgames.find(g_itr->id);
                     _currentgames.modify(cg_itr,_self,[&](auto &cg){
                        cg.steps +=1; 
                        cg.lasttime=now();
                     });
       
                }else{
                    //step　＝　５　说明　　creater最后一次押注，押注后需要计算双方得分，以及双方投入，并确定最终胜者，分发奖励。
                    uint32_t pscore = g_itr->pscore;
                    uint32_t cscore = g_itr->cscore + score;

                    uint64_t gameid = g_itr->id;
                    int ppoint = pscore-21;
                    if(ppoint<0){
                        ppoint = 21-pscore;
                    }
                    int cpoint = cscore-21;
                    if(cpoint<0){
                        cpoint = 21-cscore;
                    }

                    if( ppoint < cpoint){
                        //player 胜
                        asset playerpayout = (g_itr->ctotalpay + quantity)/100*90 + g_itr->ptotalpay;    //奖励90%的creater投入以及自己的全部投入 ，10%开发维护费用。 
                        asset devfee = (g_itr->ctotalpay + quantity)/10;        //10%合约开发维护费用
                        asset createrpayout =  g_itr->deposit; //游戏押金。
                        action(
                            permission_level{_self,N(active)},
                            TOKEN_CONTRACT,N(transfer),
                            make_tuple(_self,g_itr->player,playerpayout,string("winner of battle game! reward from battle.ejc.cash!"))
                        ).send();      

                        action(
                            permission_level{_self,N(active)},
                            TOKEN_CONTRACT,N(transfer),
                            make_tuple(_self,DEV_FEE_ACCOUNT,devfee,string("dev fee from battle.ejc.cash!"))
                        ).send();

                        action(
                            permission_level{_self,N(active)},
                            TOKEN_CONTRACT,N(transfer),
                            make_tuple(_self,g_itr->creater,createrpayout,string("deposit from battle.ejc.cash!"))
                        ).send();

                        //更新creater游戏信息
                        auto c_itr = _players.find( g_itr->creater);
                        if(c_itr == _players.end()){
                            _players.emplace(_self,[&](auto &p){                                
                                p.account =  g_itr->creater;
                                p.lost = 1;
                                p.win = 0;
                                p.draw = 0;
                                p.pay = g_itr->ctotalpay + quantity;
                                p.payout = asset(0,GAME_SYMBOL);
                            });
                        }else{
                            _players.modify(c_itr,_self,[&](auto &p){
                                p.lost+=1;
                                p.pay+= (g_itr->ctotalpay + quantity);
                            });
                        }

                        //更新player游戏信息
                        auto p_itr = _players.find(g_itr->player);
                        if(p_itr == _players.end()){
                            _players.emplace(_self,[&](auto &p){
                                p.account = g_itr->player;
                                p.lost =0;
                                p.win =1;
                                p.draw =0;
                                p.pay = g_itr->ptotalpay;
                                p.payout = playerpayout;
                            });
                        }else{
                             _players.modify(p_itr,_self,[&](auto &p){
                                p.win += 1;
                                p.pay += g_itr->ptotalpay;
                                p.payout += playerpayout;
                             });
                        }


                    }else if( ppoint > cpoint){
                        //creater 胜
                        asset devfee = g_itr->ptotalpay/10;        //10%合约开发维护费用
                        asset createrpayout = g_itr->ptotalpay/100*90 + g_itr->deposit + g_itr->ctotalpay + quantity ; //player的90% ＋ 押金+投入
             
                        action(
                            permission_level{_self,N(active)},
                            TOKEN_CONTRACT,N(transfer),
                            make_tuple(_self,DEV_FEE_ACCOUNT,devfee,string("dev fee from battle.ejc.cash!"))
                        ).send();

                        action(
                            permission_level{_self,N(active)},
                            TOKEN_CONTRACT,N(transfer),
                            make_tuple(_self,g_itr->creater,createrpayout,string("winner of battle game! reward from battle.ejc.cash!"))
                        ).send();


                          //更新creater游戏信息
                        auto c_itr = _players.find( g_itr->creater);
                        if(c_itr == _players.end()){
                            _players.emplace(_self,[&](auto &p){                                
                                p.account =  g_itr->creater;
                                p.lost = 0;
                                p.win = 1;
                                p.draw = 0;
                                p.pay = g_itr->ctotalpay + quantity;
                                p.payout = createrpayout - g_itr->deposit;   //押金不计入总投入，自然也不计入总收入。
                            });
                        }else{
                            _players.modify(c_itr,_self,[&](auto &p){
                                p.win+=1;
                                p.pay+= (g_itr->ctotalpay + quantity);
                                p.payout += (createrpayout - g_itr->deposit); //同上收入中要排除押金。
                            });
                        }

                        //更新player游戏信息
                        auto p_itr = _players.find(g_itr->player);
                        if(p_itr == _players.end()){
                            _players.emplace(_self,[&](auto &p){
                                p.account = g_itr->player;
                                p.lost =1;
                                p.win =0;
                                p.draw =0;
                                p.pay = g_itr->ptotalpay;
                                p.payout = asset(0,GAME_SYMBOL);
                            });
                        }else{
                             _players.modify(p_itr,_self,[&](auto &p){
                                p.lost += 1;
                                p.pay += g_itr->ptotalpay;
                             });
                        }


                    }else{
                        //平
                         action(
                            permission_level{_self,N(active)},
                            TOKEN_CONTRACT,N(transfer),
                            make_tuple(_self,g_itr->player,g_itr->ptotalpay,string("draw! payout from battle.ejc.cash!"))
                        ).send();
                        action(
                            permission_level{_self,N(active)},
                            TOKEN_CONTRACT,N(transfer),
                            make_tuple(_self,g_itr->creater,g_itr->ctotalpay+quantity+g_itr->deposit,string("draw! payout from battle.ejc.cash!"))
                        ).send();


                          //更新creater游戏信息
                        auto c_itr = _players.find( g_itr->creater);
                        if(c_itr == _players.end()){
                            _players.emplace(_self,[&](auto &p){                                
                                p.account =  g_itr->creater;
                                p.lost = 0;
                                p.win = 0;
                                p.draw = 1;
                                p.pay = g_itr->ctotalpay + quantity;
                                p.payout = g_itr->ctotalpay + quantity;   //押金不计入总投入，自然也不计入总收入。
                            });
                        }else{
                            _players.modify(c_itr,_self,[&](auto &p){
                                p.draw+=1;
                                p.pay+= ( g_itr->ctotalpay + quantity);
                                p.payout += (g_itr->ctotalpay + quantity); //同上收入中要排除押金。
                            });
                        }

                        //更新player游戏信息
                        auto p_itr = _players.find(g_itr->player);
                        if(p_itr == _players.end()){
                            _players.emplace(_self,[&](auto &p){
                                p.account = g_itr->player;
                                p.lost =0;
                                p.win =0;
                                p.draw =1;
                                p.pay = g_itr->ptotalpay;
                                p.payout = g_itr->ptotalpay;
                            });
                        }else{
                             _players.modify(p_itr,_self,[&](auto &p){
                                p.draw += 1;
                                p.pay += g_itr->ptotalpay;
                                p.payout += g_itr->ptotalpay;
                             });
                        }
                    }


                    //更新游戏信息
                    _games.modify(g_itr,_self,[&](auto &g){
                        g.ctotalpay+=quantity;
                        if(ppoint > cpoint){
                            g.winner = g.creater;
                        }else if(ppoint < cpoint){
                            g.winner=g.player;
                        }
                        g.cscore += score;
                        g.steps +=1;
                        g.lastpay = quantity;
                        g.lasttime = now();
                        g.status = 2; //表示游戏已结束；
                        
                    });

                    //从正在进行游戏列表中将游戏清除
                    auto cg_itr = _currentgames.find(gameid);
                    if(cg_itr != _currentgames.end()){
                        _currentgames.erase(cg_itr);
                    }
                }
            }else if(g_itr->steps ==0 || g_itr->steps == 2 || g_itr->steps ==4){
                uint32_t score = getscore(); //得到本次得分
                //此时为player支付
                if(g_itr->status !=0){
                    //如果不为０ 需要检查当前支付人是否为player(防止是creater)
                    eosio_assert(from == g_itr->player,"it's not your turn  yet");
                    _games.modify(g_itr,_self,[&](auto &g){
                        g.ptotalpay += quantity;
                        g.pscore += score;
                        g.steps +=1;
                        g.lastpay = quantity;
                        g.lasttime= now();
                    });   
                    auto cg_itr = _currentgames.find(g_itr->id);
                     _currentgames.modify(cg_itr,_self,[&](auto &cg){
                        cg.steps +=1; 
                        cg.lasttime=now();
                     }); 

                }else{
                    //如果为０ 说明首次参与，上边已做出判断不能为creater 此处不需要再进行判断                    
                    _games.modify(g_itr,_self,[&](auto &g){
                        g.player = from;                
                        g.ptotalpay += quantity;
                        g.pscore += score;
                        g.steps +=1;
                        g.lastpay = quantity;
                        g.lasttime= now();
                        g.status = 1;                        
                    });
                    auto cg_itr = _currentgames.find(g_itr->id);
                    _currentgames.modify(cg_itr,_self,[&](auto &cg){            
                    cg.player=from;                        
                    cg.status = 1;
                    cg.steps = 1; 
                    cg.lasttime=now();
                    });
                }
            }               

        }
    }

};


#define EOSIO_ABI_PRO(TYPE, MEMBERS)                                                                                                              \
  extern "C" {                                                                                                                                    \
  void apply(uint64_t receiver, uint64_t code, uint64_t action)                                                                                   \
  {                                                                                                                                               \
    auto self = receiver;                                                                                                                         \
    if (action == N(onerror))                                                                                                                     \
    {                                                                                                                                             \
      eosio_assert(code == N(eosio), "onerror action's are only valid from the \"eosio\" system account");                                        \
    }                                                                                                                                             \
    if ((code == TOKEN_CONTRACT && action == N(transfer)) ||(code == self && (action == N(giveup) || action == N(claim) || action == N(closegame)   \
                                  || action == N(creategame)  || action == N(roll) || action == N(cleargame) || action == N(clear)   )))                                                     \
    {                                                                                                                                             \
        TYPE thiscontract(self);                                                                                                                    \
        switch (action)                                                                                                                             \
        {                                                                                                                                           \
            EOSIO_API(TYPE, MEMBERS)                                                                                                                  \
        }                                                                                                                                           \
    }                                                                                                                                             \
  }                                                                                                                                               \
  }

EOSIO_ABI_PRO(playerbattle, (transfer)(giveup)(claim)(cleargame)(closegame)(creategame)(roll)(clear))