class Users::ProfilesController < ApplicationController

  def index
  end

  def new
    if current_user.profile
      redirect_to users_profile_path
    else
      @profile = Profile.new #current_user.build_profile
    end
  end

  def create
    @profile = current_user.build_profile(params[:profile])

    respond_to do |format|
      @profile.work_location = params[:addr]

      if  @profile.save

        format.html { redirect_to ([:users,@profile]) ,:notice => "Profile Saved Successfully." }
      else
        format.html { render :action => "new" }
        format.xml  { render :xml => @profile.errors, :status => :unprocessable_entity }
      end
    end
  end

  def edit
    @profile = Profile.find(params[:id])
  end

  def update

    @profile = Profile.find(params[:id])
    @profile.work_location = params[:addr]
    @profile.dob = params[:dob]
    if  @profile.update_attributes(params[:profile])
      redirect_to([:users,@profile])
      flash[:notice] = "Profile Updated Successfully."
    else
      render :action => "edit"
    end
  end

  def show
    @profile = Profile.find(params[:id])
    @json = Profile.where(:user_id=>current_user.id).to_gmaps4rails
  end

  def destroy
  end



end
