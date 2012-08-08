class Users::DashboardController < ApplicationController

  require 'gdata'

  def index

    @properties = Property.all
    @user_properties = UserProperty.where(:user_id => current_user.id)
    @looking_fors = LookingFor.all
    @ownership_types = OwnershipType.all

  end

  def invite_friends
  end

  def import
    @sites = {"hotmail" => Contacts::Hotmail,"yahoo" => Contacts::Yahoo, "gmail" => Contacts::Gmail}

    @contacts = @sites[params[:from]].new(params[:login], params[:password]).contacts

    respond_to do |format|
      format.html {render :action => 'list'}

    end

    @user_contacts = UserContact.all

  end

  def create_contacts
    @var = params[:email]
    @var.each do |email|
      contact = email.split(',')
      @ivar = UserContact.new(:name => contact[0].delete('[""'),:email =>contact[1].delete(']""') ,:user_id=>1)
      @ivar.save
    end
  end

end
